// CameraViewModel.swift
// Main view model for the camera screen.
// Owns the stream client, recording service, screenshot service and doorbell polling.
//
// RECORDING RULES enforced here:
//   - isRecording starts as false on every instantiation
//   - No code path other than startRecording() sets isRecording to true
//   - App lifecycle (background/foreground) does NOT auto-start recording
import Foundation
import SwiftUI
import Photos

enum ConnectionState {
    case idle, connecting, connected, disconnected, failed(Error)

    var label: String {
        switch self {
        case .idle:         return "Idle"
        case .connecting:   return "Connecting…"
        case .connected:    return "Live"
        case .disconnected: return "Disconnected"
        case .failed:       return "Error"
        }
    }

    var isConnected: Bool {
        if case .connected = self { return true }
        return false
    }
}

@Observable
@MainActor
final class CameraViewModel {
    // ---- Connection state
    var connectionState: ConnectionState = .idle
    var currentFrame: UIImage?

    // ---- Recording state (never auto-start)
    private(set) var isRecording: Bool = false
    var recordingDuration: TimeInterval = 0
    var recordingError: String?

    // ---- Screenshot state
    var screenshotResult: Result<URL, Error>?
    var showShareSheet: Bool = false
    var shareURL: URL?

    // ---- Doorbell
    var latestDoorbellEvent: DoorbellEvent?
    var showDoorbellAlert: Bool = false

    // ---- Settings
    var settings: CameraSettings = .defaultSettings

    // ---- Services
    private var streamClient    = MJPEGStreamClient()
    private var recordingService = RecordingService()
    private var screenshotService = ScreenshotService()
    private let doorbellService   = DoorbellNotificationService()
    private var apiClient: CameraAPIClient?
    private var device: CameraDevice?

    // ---- Reconnect timer
    private var reconnectTask: Task<Void, Never>?
    private var durationTimer: Timer?

    // MARK: – Lifecycle

    func connect(to device: CameraDevice) {
        self.device    = device
        self.apiClient = CameraAPIClient(device: device)
        recordingService.setDeviceId(device.deviceId)
        screenshotService.deviceId = device.deviceId

        // Wire stream callbacks
        streamClient.onStateChange = { [weak self] state in
            Task { @MainActor [weak self] in
                self?.handleStreamState(state)
            }
        }
        streamClient.onFrame = { [weak self] frame in
            Task { @MainActor [weak self] in
                self?.handleFrame(frame)
            }
        }

        // Wire doorbell polling
        doorbellService.onNewEvent = { [weak self] event in
            Task { @MainActor [weak self] in
                self?.latestDoorbellEvent = event
                self?.showDoorbellAlert  = true
            }
        }
        if let client = apiClient {
            doorbellService.start(apiClient: client)
        }

        startStream()
        Task { await loadSettings() }
    }

    func disconnect() {
        reconnectTask?.cancel()
        durationTimer?.invalidate()
        streamClient.disconnect()
        doorbellService.stop()
        connectionState = .idle
        // Recording is NOT stopped automatically on disconnect – caller decides
    }

    // MARK: – Stream control

    private func startStream() {
        guard let device = device else { return }
        connectionState = .connecting
        streamClient.connect(to: device.streamURL)
    }

    private func handleStreamState(_ state: MJPEGStreamClient.State) {
        switch state {
        case .streaming:
            connectionState = .connected
            reconnectTask?.cancel()
        case .failed(let e):
            connectionState = .failed(e)
            scheduleReconnect()
        case .disconnected:
            connectionState = .disconnected
            scheduleReconnect()
        case .connecting:
            connectionState = .connecting
        case .idle:
            connectionState = .idle
        }
    }

    private func scheduleReconnect() {
        reconnectTask?.cancel()
        reconnectTask = Task { [weak self] in
            try? await Task.sleep(nanoseconds: 3_000_000_000)
            guard !Task.isCancelled else { return }
            await MainActor.run { self?.startStream() }
        }
    }

    private func handleFrame(_ frame: MJPEGFrame) {
        // Update displayed image
        if let img = UIImage(data: frame.jpegData) {
            currentFrame = img
        }

        // Feed to recorder if active
        if isRecording {
            recordingService.appendFrame(
                jpegData: frame.jpegData,
                presentationTime: frame.timestamp)
        }
    }

    // MARK: – Recording

    /// Explicit user action required. Never called automatically.
    func startRecording() {
        guard !isRecording else { return }
        do {
            try recordingService.startRecording()
            isRecording    = true
            recordingError = nil
            startDurationTimer()
        } catch {
            recordingError = error.localizedDescription
        }
    }

    /// Explicit user action required.
    func stopRecording() async {
        guard isRecording else { return }
        isRecording = false
        stopDurationTimer()
        do {
            let url = try await recordingService.stopRecording()
            // Save to Photos; fall back to share sheet
            await saveVideoToPhotos(url: url)
        } catch {
            recordingError = error.localizedDescription
        }
    }

    private func saveVideoToPhotos(url: URL) async {
        let status = PHPhotoLibrary.authorizationStatus(for: .addOnly)
        var authGranted = status == .authorized || status == .limited
        if status == .notDetermined {
            authGranted = await PHPhotoLibrary.requestAuthorization(for: .addOnly) == .authorized
        }
        if authGranted {
            do {
                try await withCheckedThrowingContinuation { (cont: CheckedContinuation<Void, Error>) in
                    PHPhotoLibrary.shared().performChanges({
                        PHAssetChangeRequest.creationRequestForAssetFromVideo(atFileURL: url)
                    }) { success, error in
                        if success { cont.resume() }
                        else { cont.resume(throwing: error ?? URLError(.unknown)) }
                    }
                }
            } catch {
                recordingError = error.localizedDescription
            }
        } else {
            // Present share sheet
            shareURL        = url
            showShareSheet  = true
        }
    }

    // MARK: – Screenshot

    func takeScreenshot() async {
        // Prefer the high-quality snapshot endpoint
        let jpegData: Data
        do {
            if let client = apiClient {
                jpegData = try await client.snapshot()
            } else if let img = currentFrame,
                      let data = img.jpegData(compressionQuality: 0.9) {
                jpegData = data
            } else {
                screenshotResult = .failure(ScreenshotError.noFrame)
                return
            }
            let url = try await screenshotService.save(jpegData: jpegData)
            screenshotResult = .success(url)
        } catch {
            screenshotResult = .failure(error)
            // If Photos is denied, surface the share sheet
            if error is ScreenshotError {
                shareURL       = (screenshotResult.flatMap { if case .success(let u) = $0 { return u } else { return nil } })
                showShareSheet = shareURL != nil
            }
        }
    }

    // MARK: – Settings

    func loadSettings() async {
        guard let client = apiClient else { return }
        do {
            settings = try await client.cameraSettings()
        } catch {
            // Not fatal – use cached defaults
        }
    }

    // MARK: – App lifecycle

    func handleBackground() {
        // Stop recording safely if active – never continue recording in background
        if isRecording {
            Task { await stopRecording() }
        }
    }

    // MARK: – Duration timer

    private func startDurationTimer() {
        durationTimer?.invalidate()
        durationTimer = Timer.scheduledTimer(withTimeInterval: 0.5, repeats: true) { [weak self] _ in
            Task { @MainActor [weak self] in
                self?.recordingService.updateDuration()
                self?.recordingDuration = self?.recordingService.duration ?? 0
            }
        }
    }

    private func stopDurationTimer() {
        durationTimer?.invalidate()
        durationTimer = nil
        recordingDuration = 0
    }
}
