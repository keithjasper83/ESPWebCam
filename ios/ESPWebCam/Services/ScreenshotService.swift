// ScreenshotService.swift
// Saves a JPEG frame to the iPhone Photos library (or share sheet if denied).
import Foundation
import Photos
import UIKit

enum ScreenshotError: LocalizedError {
    case noFrame
    case saveFailed(Error)
    var errorDescription: String? {
        switch self {
        case .noFrame:          return "No camera frame available"
        case .saveFailed(let e): return "Save failed: \(e.localizedDescription)"
        }
    }
}

@MainActor
final class ScreenshotService {
    var deviceId: String = "espwebcam"

    /// Save a JPEG from the given Data.
    /// Returns the URL of the saved file (temp directory copy).
    func save(jpegData: Data) async throws -> URL {
        let url = makeFileURL()
        try jpegData.write(to: url)

        let status = PHPhotoLibrary.authorizationStatus(for: .addOnly)
        if status == .authorized || status == .limited {
            try await saveToPhotos(url: url)
        } else if status == .notDetermined {
            let granted = await PHPhotoLibrary.requestAuthorization(for: .addOnly) == .authorized
            if granted {
                try await saveToPhotos(url: url)
            }
            // If denied, the caller will present a share sheet using the returned URL
        }
        // If denied/restricted, caller uses the URL for a share sheet
        return url
    }

    private func saveToPhotos(url: URL) async throws {
        try await withCheckedThrowingContinuation { (cont: CheckedContinuation<Void, Error>) in
            PHPhotoLibrary.shared().performChanges({
                let req = PHAssetChangeRequest.creationRequestForAssetFromImage(atFileURL: url)
                _ = req  // suppress unused warning
            }) { success, error in
                if success {
                    cont.resume()
                } else {
                    cont.resume(throwing: ScreenshotError.saveFailed(error ?? URLError(.unknown)))
                }
            }
        }
    }

    private func makeFileURL() -> URL {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd_HH-mm-ss"
        formatter.timeZone = .current
        let filename = "ESPWebCam_\(formatter.string(from: Date())).jpg"
        return FileManager.default.temporaryDirectory.appendingPathComponent(filename)
    }
}
