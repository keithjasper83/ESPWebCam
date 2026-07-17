// MJPEGFrameParserTests.swift
// Unit tests for MJPEGFrameParser using recorded byte fixtures.
// Run with ⌘U in Xcode.
import XCTest
@testable import ESPWebCam

final class MJPEGFrameParserTests: XCTestCase {
    private let boundary = "--123456789000000000000987654321"

    // MARK: – Helpers

    func makePart(jpeg: Data, timestamp: String? = nil) -> Data {
        var part = Data()
        part.append(contentsOf: (boundary + "\r\n").utf8)
        part.append(contentsOf: "Content-Type: image/jpeg\r\n".utf8)
        part.append(contentsOf: "Content-Length: \(jpeg.count)\r\n".utf8)
        if let ts = timestamp {
            part.append(contentsOf: "X-Timestamp: \(ts)\r\n".utf8)
        }
        part.append(contentsOf: "\r\n".utf8)
        part.append(jpeg)
        return part
    }

    func minimalJPEG() -> Data {
        // Minimal valid JPEG SOI marker bytes
        return Data([0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x01, 0xFF, 0xD9])
    }

    // MARK: – Tests

    func testParsesCompleteFrame() throws {
        let jpeg    = minimalJPEG()
        let stream  = makePart(jpeg: jpeg, timestamp: "1000.500000")
        let parser  = MJPEGFrameParser()

        var frames: [MJPEGFrame] = []
        parser.onFrame = { frames.append($0) }
        parser.append(stream)

        XCTAssertEqual(frames.count, 1)
        XCTAssertEqual(frames[0].jpegData, jpeg)
        XCTAssertEqual(frames[0].timestamp, 1000.5, accuracy: 0.001)
    }

    func testParsesMultipleFrames() {
        let jpeg1  = Data([0xFF, 0xD8, 0x01, 0xD9])
        let jpeg2  = Data([0xFF, 0xD8, 0x02, 0xD9])
        var stream = makePart(jpeg: jpeg1)
        stream.append(makePart(jpeg: jpeg2))

        let parser = MJPEGFrameParser()
        var frames: [MJPEGFrame] = []
        parser.onFrame = { frames.append($0) }
        parser.append(stream)

        XCTAssertEqual(frames.count, 2)
        XCTAssertEqual(frames[0].jpegData, jpeg1)
        XCTAssertEqual(frames[1].jpegData, jpeg2)
    }

    func testHandlesPartialRead() {
        let jpeg   = minimalJPEG()
        let stream = makePart(jpeg: jpeg)

        let parser = MJPEGFrameParser()
        var frames: [MJPEGFrame] = []
        parser.onFrame = { frames.append($0) }

        // Feed in 4-byte chunks
        var offset = 0
        while offset < stream.count {
            let end = min(offset + 4, stream.count)
            parser.append(stream[offset..<end])
            offset = end
        }

        XCTAssertEqual(frames.count, 1)
        XCTAssertEqual(frames[0].jpegData, jpeg)
    }

    func testEmptyInputProducesNoFrames() {
        let parser = MJPEGFrameParser()
        var frames: [MJPEGFrame] = []
        parser.onFrame = { frames.append($0) }
        parser.append(Data())
        XCTAssertEqual(frames.count, 0)
    }

    func testMalformedBoundaryProducesNoFrames() {
        let parser = MJPEGFrameParser()
        var frames: [MJPEGFrame] = []
        parser.onFrame = { frames.append($0) }
        parser.append(Data("this is not a valid multipart stream".utf8))
        XCTAssertEqual(frames.count, 0)
    }

    func testResetClearsState() {
        let jpeg   = minimalJPEG()
        let half   = makePart(jpeg: jpeg).prefix(10)
        let parser = MJPEGFrameParser()
        var frames: [MJPEGFrame] = []
        parser.onFrame = { frames.append($0) }
        parser.append(Data(half))
        parser.reset()
        // After reset, a fresh complete frame should parse correctly
        parser.append(makePart(jpeg: jpeg))
        XCTAssertEqual(frames.count, 1)
    }

    func testFrameWithoutTimestampUsesArrivalTime() {
        let jpeg   = minimalJPEG()
        let stream = makePart(jpeg: jpeg, timestamp: nil)
        let before = Date().timeIntervalSinceReferenceDate
        let parser = MJPEGFrameParser()
        var frames: [MJPEGFrame] = []
        parser.onFrame = { frames.append($0) }
        parser.append(stream)
        let after = Date().timeIntervalSinceReferenceDate
        XCTAssertEqual(frames.count, 1)
        XCTAssert(frames[0].timestamp >= before && frames[0].timestamp <= after)
    }
}
