// CameraSettings.swift – API camera settings model
import Foundation

/// Camera settings as returned by GET /api/v1/camera/settings
/// and accepted by PUT /api/v1/camera/settings.
/// Field names match the OpenAPI schema exactly.
struct CameraSettings: Codable, Equatable {
    var frameSize: Int
    var frameSizeName: String
    var jpegQuality: Int
    var brightness: Int
    var contrast: Int
    var saturation: Int
    var aeLevel: Int
    var hmirror: Int
    var vflip: Int
    var sensorPid: String

    static let defaultSettings = CameraSettings(
        frameSize: 8, frameSizeName: "VGA", jpegQuality: 12,
        brightness: 1, contrast: 0, saturation: 0, aeLevel: -3,
        hmirror: 1, vflip: 0, sensorPid: "unknown"
    )
}

/// Subset used for a PUT request (all fields optional in the API).
struct CameraSettingsRequest: Encodable {
    var frameSize: Int?
    var jpegQuality: Int?
    var brightness: Int?
    var contrast: Int?
    var saturation: Int?
    var aeLevel: Int?
    var hmirror: Int?
    var vflip: Int?
}

// MARK: – API response envelopes

struct APIResponse<T: Decodable>: Decodable {
    let ok: Bool
    let data: T?
    let error: APIError?
}

struct APIError: Decodable {
    let code: String
    let message: String
}
