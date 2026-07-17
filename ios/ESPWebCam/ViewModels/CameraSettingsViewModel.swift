// CameraSettingsViewModel.swift
import Foundation

@Observable
@MainActor
final class CameraSettingsViewModel {
    var settings: CameraSettings = .defaultSettings
    var isSaving: Bool = false
    var errorMessage: String?
    var successMessage: String?

    private var apiClient: CameraAPIClient?

    func configure(device: CameraDevice) {
        apiClient = CameraAPIClient(device: device)
    }

    func load() async {
        guard let client = apiClient else { return }
        do {
            settings = try await client.cameraSettings()
        } catch {
            errorMessage = error.localizedDescription
        }
    }

    func save() async {
        guard let client = apiClient else { return }
        isSaving = true
        errorMessage = nil
        defer { isSaving = false }

        let req = CameraSettingsRequest(
            frameSize:    settings.frameSize,
            jpegQuality:  settings.jpegQuality,
            brightness:   settings.brightness,
            contrast:     settings.contrast,
            saturation:   settings.saturation,
            aeLevel:      settings.aeLevel,
            hmirror:      settings.hmirror,
            vflip:        settings.vflip
        )
        do {
            settings       = try await client.updateCameraSettings(req)
            successMessage = "Settings applied"
        } catch {
            errorMessage = error.localizedDescription
        }
    }
}
