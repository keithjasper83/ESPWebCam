// CameraDevice.swift – persisted camera identity
import Foundation

/// A discovered or manually-entered camera device.
struct CameraDevice: Identifiable, Codable, Equatable, Hashable {
    let id: UUID
    var name: String        // User-visible label, e.g. "Front Door"
    var host: String        // hostname or IP, e.g. "espwebcam.local" or "192.168.1.100"
    var httpPort: UInt16    // API port (default 80)
    var streamPort: UInt16  // MJPEG stream port (default 81)
    var deviceId: String    // Device ID from /api/v1/status

    init(id: UUID = UUID(), name: String, host: String,
         httpPort: UInt16 = 80, streamPort: UInt16 = 81,
         deviceId: String = "") {
        self.id         = id
        self.name       = name
        self.host       = host
        self.httpPort   = httpPort
        self.streamPort = streamPort
        self.deviceId   = deviceId
    }

    var apiBaseURL: URL {
        URL(string: "http://\(host):\(httpPort)/api/v1")!
    }

    var streamURL: URL {
        URL(string: "http://\(host):\(streamPort)/stream")!
    }
}

// MARK: – CameraStore

/// Observable store that persists the list of known cameras.
@Observable
final class CameraStore {
    private(set) var cameras: [CameraDevice] = []
    private let key = "savedCameras"

    init() {
        load()
        if cameras.isEmpty {
            // Add a default placeholder so the UI is never completely empty
            cameras = [CameraDevice(name: "ESP Camera", host: "espwebcam.local")]
        }
    }

    func add(_ device: CameraDevice) {
        cameras.append(device)
        save()
    }

    func remove(_ device: CameraDevice) {
        cameras.removeAll { $0.id == device.id }
        save()
    }

    func update(_ device: CameraDevice) {
        if let i = cameras.firstIndex(where: { $0.id == device.id }) {
            cameras[i] = device
            save()
        }
    }

    private func save() {
        if let data = try? JSONEncoder().encode(cameras) {
            UserDefaults.standard.set(data, forKey: key)
        }
    }

    private func load() {
        guard let data = UserDefaults.standard.data(forKey: key),
              let decoded = try? JSONDecoder().decode([CameraDevice].self, from: data) else { return }
        cameras = decoded
    }
}
