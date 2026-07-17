// MJPEGFrameParser.swift
// Parses a multipart/x-mixed-replace MJPEG stream into individual JPEG frames.
//
// Designed to be independently unit-tested with recorded byte fixtures.
// See ESPWebCamTests/MJPEGFrameParserTests.swift.
import Foundation

struct MJPEGFrame {
    let jpegData: Data
    let timestamp: TimeInterval   // from X-Timestamp header, or arrival time
}

/// Feed arbitrary chunks of stream data via `append(_:)`.
/// Complete JPEG frames are delivered via the `onFrame` callback on the same
/// thread/actor as the caller.
final class MJPEGFrameParser {
    // The boundary string sent by the ESP firmware
    private static let boundary = "--123456789000000000000987654321"
    private static let boundaryData = Data((boundary + "\r\n").utf8)
    private static let crlfcrlf = Data("\r\n\r\n".utf8)

    var onFrame: ((MJPEGFrame) -> Void)?

    private var buffer = Data()
    private var pendingContentLength: Int?
    private var pendingTimestamp: TimeInterval?

    func append(_ chunk: Data) {
        buffer.append(chunk)
        processBuffer()
    }

    func reset() {
        buffer.removeAll()
        pendingContentLength = nil
        pendingTimestamp     = nil
    }

    // MARK: – Private

    private func processBuffer() {
        while true {
            // 1. Find the boundary
            guard let boundaryRange = buffer.range(of: Self.boundaryData) else {
                // No complete boundary yet; keep buffering.
                // Trim anything before a partial boundary match to limit growth.
                if buffer.count > 65536 {
                    // Drop old data that can't possibly contain a valid frame
                    if let partial = lastPartialBoundaryStart() {
                        buffer = buffer.subdata(in: partial ..< buffer.endIndex)
                    } else {
                        buffer.removeAll()
                    }
                }
                return
            }

            // Drop everything before the boundary
            buffer = buffer.subdata(in: boundaryRange.upperBound ..< buffer.endIndex)

            // 2. Find the end of the headers (\r\n\r\n)
            guard let headerEnd = buffer.range(of: Self.crlfcrlf) else {
                return  // Incomplete headers – wait for more data
            }

            let headerData = buffer.subdata(in: buffer.startIndex ..< headerEnd.lowerBound)
            let headers    = parseHeaders(headerData)

            let contentLength: Int
            if let cl = headers["content-length"].flatMap(Int.init) {
                contentLength = cl
            } else {
                // No Content-Length; skip this part
                buffer = buffer.subdata(in: headerEnd.upperBound ..< buffer.endIndex)
                continue
            }

            let bodyStart  = headerEnd.upperBound
            let bodyEnd    = bodyStart + contentLength
            guard buffer.count >= bodyEnd else {
                return  // Frame body not fully received yet
            }

            let jpegData = buffer.subdata(in: bodyStart ..< bodyEnd)
            let timestamp: TimeInterval
            if let tsStr = headers["x-timestamp"],
               let ts = parseXTimestamp(tsStr) {
                timestamp = ts
            } else {
                timestamp = Date().timeIntervalSinceReferenceDate
            }

            onFrame?(MJPEGFrame(jpegData: jpegData, timestamp: timestamp))

            // Advance past the consumed frame
            buffer = buffer.subdata(in: bodyEnd ..< buffer.endIndex)
        }
    }

    private func parseHeaders(_ data: Data) -> [String: String] {
        guard let str = String(data: data, encoding: .utf8) else { return [:] }
        var dict: [String: String] = [:]
        for line in str.components(separatedBy: "\r\n") {
            let parts = line.split(separator: ":", maxSplits: 1)
            if parts.count == 2 {
                let key   = parts[0].trimmingCharacters(in: .whitespaces).lowercased()
                let value = parts[1].trimmingCharacters(in: .whitespaces)
                dict[key] = value
            }
        }
        return dict
    }

    /// Parse "X-Timestamp: 1234567890.000000" → TimeInterval
    private func parseXTimestamp(_ value: String) -> TimeInterval? {
        let parts = value.split(separator: ".")
        guard parts.count == 2,
              let sec  = Double(parts[0]),
              let usec = Double(parts[1]) else { return nil }
        return sec + usec / 1_000_000
    }

    /// Find the start of a possible partial boundary at the end of the buffer
    private func lastPartialBoundaryStart() -> Data.Index? {
        let bPrefix = Self.boundary.prefix(4)
        let bData   = Data(bPrefix.utf8)
        // Search backwards for the start of a partial boundary
        for i in stride(from: buffer.endIndex - 1, through: buffer.startIndex, by: -1) {
            if buffer[i] == bData[0] {
                return i
            }
        }
        return nil
    }
}
