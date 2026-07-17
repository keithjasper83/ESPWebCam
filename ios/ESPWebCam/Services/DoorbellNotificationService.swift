// DoorbellNotificationService.swift
// Polls /api/v1/doorbell/status and fires a callback when a new event is detected.
import Foundation

@MainActor
final class DoorbellNotificationService {
    var onNewEvent: ((DoorbellEvent) -> Void)?

    private var apiClient: CameraAPIClient?
    private var lastSeenSequence: UInt32 = 0
    private var pollTask: Task<Void, Never>?

    func start(apiClient: CameraAPIClient) {
        self.apiClient = apiClient
        startPolling()
    }

    func stop() {
        pollTask?.cancel()
        pollTask = nil
    }

    private func startPolling() {
        pollTask?.cancel()
        pollTask = Task { [weak self] in
            while !Task.isCancelled {
                await self?.poll()
                try? await Task.sleep(nanoseconds: 3_000_000_000)  // 3 s
            }
        }
    }

    private func poll() async {
        guard let client = apiClient else { return }
        do {
            let status = try await client.doorbellStatus()
            if let event = status.lastEvent,
               event.sequence != lastSeenSequence {
                lastSeenSequence = event.sequence
                onNewEvent?(event)
            }
        } catch {
            // Ignore poll errors – connection may be temporarily unavailable
        }
    }
}
