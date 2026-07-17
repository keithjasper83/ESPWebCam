// RecordingService.swift
// Phone-side video recording using AVFoundation.
//
// RECORDING RULES (strictly enforced):
//   • Recording NEVER starts automatically.
//   • Recording starts ONLY when startRecording() is called explicitly.
//   • Doorbell presses, app launch, reconnects and timers do NOT start recording.
//   • Recording is stopped and finalised when stopRecording() is called,
//     or when the app enters the background.
import Foundation
import AVFoundation
import UIKit

enum RecordingError: LocalizedError {
    case alreadyRecording
    case notRecording
    case writerSetupFailed(Error)
    case inputSetupFailed
    case finalisationFailed(Error)

    var errorDescription: String? {
        switch self {
        case .alreadyRecording:           return "Recording is already in progress"
        case .notRecording:               return "No active recording"
        case .writerSetupFailed(let e):   return "Writer setup failed: \(e.localizedDescription)"
        case .inputSetupFailed:           return "Asset writer input setup failed"
        case .finalisationFailed(let e):  return "Finalisation failed: \(e.localizedDescription)"
        }
    }
}

@MainActor
final class RecordingService {
    private(set) var isRecording = false
    private(set) var startTime: Date?
    private(set) var duration: TimeInterval = 0

    // Called from the view model's timer
    func updateDuration() {
        guard let start = startTime else { return }
        duration = Date().timeIntervalSince(start)
    }

    private var assetWriter: AVAssetWriter?
    private var videoInput: AVAssetWriterInput?
    private var pixelBufferAdaptor: AVAssetWriterInputPixelBufferAdaptor?
    private var outputURL: URL?
    private var firstFrameTime: TimeInterval?
    private var deviceId: String = "espwebcam"
    private var bgTask: UIBackgroundTaskIdentifier = .invalid

    func setDeviceId(_ id: String) { deviceId = id }

    // MARK: – Start recording

    func startRecording() throws {
        guard !isRecording else { throw RecordingError.alreadyRecording }

        let outputURL = makeOutputURL()
        self.outputURL = outputURL

        // Remove any existing file at the path
        try? FileManager.default.removeItem(at: outputURL)

        let writer: AVAssetWriter
        do {
            writer = try AVAssetWriter(url: outputURL, fileType: .mp4)
        } catch {
            throw RecordingError.writerSetupFailed(error)
        }

        // H.264 video settings
        let videoSettings: [String: Any] = [
            AVVideoCodecKey: AVVideoCodecType.h264,
            AVVideoWidthKey: 640,   // Matches VGA default; adjust if resolution changes
            AVVideoHeightKey: 480,
        ]

        let input = AVAssetWriterInput(mediaType: .video, outputSettings: videoSettings)
        input.expectsMediaDataInRealTime = true

        let attrs: [String: Any] = [
            kCVPixelBufferPixelFormatTypeKey as String: Int(kCVPixelFormatType_32BGRA),
            kCVPixelBufferWidthKey  as String: 640,
            kCVPixelBufferHeightKey as String: 480,
        ]
        let adaptor = AVAssetWriterInputPixelBufferAdaptor(
            assetWriterInput: input,
            sourcePixelBufferAttributes: attrs
        )

        guard writer.canAdd(input) else { throw RecordingError.inputSetupFailed }
        writer.add(input)

        assetWriter       = writer
        videoInput        = input
        pixelBufferAdaptor = adaptor
        firstFrameTime    = nil
        startTime         = Date()
        duration          = 0
        isRecording       = true

        writer.startWriting()
        writer.startSession(atSourceTime: .zero)
    }

    // MARK: – Append frame

    /// Append a JPEG frame received from the MJPEG stream.
    /// presentationTime should be from X-Timestamp or actual arrival time.
    func appendFrame(jpegData: Data, presentationTime: TimeInterval) {
        guard isRecording,
              let adaptor = pixelBufferAdaptor,
              let input   = videoInput,
              input.isReadyForMoreMediaData else { return }

        guard let image   = UIImage(data: jpegData),
              let cgImage = image.cgImage else { return }

        let first    = firstFrameTime ?? presentationTime
        if firstFrameTime == nil { firstFrameTime = presentationTime }
        let relativeSeconds = presentationTime - first
        let pts      = CMTime(seconds: relativeSeconds, preferredTimescale: 1_000_000)

        var pixelBuffer: CVPixelBuffer?
        let status = CVPixelBufferCreate(
            kCFAllocatorDefault,
            cgImage.width, cgImage.height,
            kCVPixelFormatType_32BGRA,
            [kCVPixelBufferCGImageCompatibilityKey: true,
             kCVPixelBufferCGBitmapContextCompatibilityKey: true] as CFDictionary,
            &pixelBuffer
        )
        guard status == kCVReturnSuccess, let pb = pixelBuffer else { return }

        CVPixelBufferLockBaseAddress(pb, [])
        let ctx = CGContext(
            data: CVPixelBufferGetBaseAddress(pb),
            width: cgImage.width, height: cgImage.height,
            bitsPerComponent: 8,
            bytesPerRow: CVPixelBufferGetBytesPerRow(pb),
            space: CGColorSpaceCreateDeviceRGB(),
            bitmapInfo: CGImageAlphaInfo.noneSkipFirst.rawValue | CGBitmapInfo.byteOrder32Little.rawValue
        )
        ctx?.draw(cgImage, in: CGRect(x: 0, y: 0, width: cgImage.width, height: cgImage.height))
        CVPixelBufferUnlockBaseAddress(pb, [])

        adaptor.append(pb, withPresentationTime: pts)
    }

    // MARK: – Stop recording

    func stopRecording() async throws -> URL {
        guard isRecording, let writer = assetWriter, let url = outputURL else {
            throw RecordingError.notRecording
        }

        // Register background task to get time to finalise
        bgTask = UIApplication.shared.beginBackgroundTask(withName: "RecordingFinalise") {
            [weak self] in self?.endBackgroundTask()
        }

        isRecording = false
        startTime   = nil
        duration    = 0
        videoInput?.markAsFinished()

        return try await withCheckedThrowingContinuation { continuation in
            writer.finishWriting {
                Task { @MainActor [weak self] in
                    self?.endBackgroundTask()
                    if writer.status == .completed {
                        continuation.resume(returning: url)
                    } else {
                        continuation.resume(throwing:
                            RecordingError.finalisationFailed(
                                writer.error ?? URLError(.unknown)))
                    }
                }
            }
        }
    }

    // MARK: – Helpers

    private func makeOutputURL() -> URL {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd_HH-mm-ss"
        let filename = "ESPWebCam_\(formatter.string(from: Date())).mp4"
        return FileManager.default.temporaryDirectory.appendingPathComponent(filename)
    }

    private func endBackgroundTask() {
        if bgTask != .invalid {
            UIApplication.shared.endBackgroundTask(bgTask)
            bgTask = .invalid
        }
    }
}
