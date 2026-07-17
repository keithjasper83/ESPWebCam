// CameraAPIClient.swift – URLSession-based REST client for /api/v1
import Foundation

enum CameraAPIError: LocalizedError {
    case networkError(Error)
    case httpError(Int)
    case apiError(code: String, message: String)
    case decodingError(Error)
    case invalidURL

    var errorDescription: String? {
        switch self {
        case .networkError(let e):      return "Network error: \(e.localizedDescription)"
        case .httpError(let code):      return "HTTP \(code)"
        case .apiError(_, let msg):     return msg
        case .decodingError(let e):     return "Decoding error: \(e.localizedDescription)"
        case .invalidURL:               return "Invalid URL"
        }
    }
}

final class CameraAPIClient {
    private let baseURL: URL
    private let session: URLSession

    init(device: CameraDevice, session: URLSession = .shared) {
        self.baseURL = device.apiBaseURL
        self.session = session
    }

    // MARK: – Health

    func health() async throws -> Bool {
        let url = baseURL.appendingPathComponent("health")
        let response: APIResponse<HealthData> = try await get(url: url)
        return response.ok
    }

    // MARK: – Status

    func status() async throws -> StatusData {
        let url = baseURL.appendingPathComponent("status")
        let response: APIResponse<StatusData> = try await get(url: url)
        return try unwrap(response)
    }

    // MARK: – Camera settings

    func cameraSettings() async throws -> CameraSettings {
        let url = baseURL.appendingPathComponent("camera/settings")
        let response: APIResponse<CameraSettings> = try await get(url: url)
        return try unwrap(response)
    }

    func updateCameraSettings(_ request: CameraSettingsRequest) async throws -> CameraSettings {
        let url = baseURL.appendingPathComponent("camera/settings")
        let body = try JSONEncoder().encode(request)
        let response: APIResponse<CameraSettings> = try await put(url: url, body: body)
        return try unwrap(response)
    }

    // MARK: – Snapshot

    /// Fetches a raw JPEG snapshot. Returns raw Data.
    func snapshot() async throws -> Data {
        let url = baseURL.appendingPathComponent("camera/snapshot")
        var req = URLRequest(url: url)
        req.timeoutInterval = 10
        let (data, httpResp) = try await session.data(for: req)
        guard let http = httpResp as? HTTPURLResponse else {
            throw CameraAPIError.networkError(URLError(.badServerResponse))
        }
        guard http.statusCode == 200 else {
            throw CameraAPIError.httpError(http.statusCode)
        }
        return data
    }

    // MARK: – Doorbell

    func doorbellStatus() async throws -> DoorbellStatus {
        let url = baseURL.appendingPathComponent("doorbell/status")
        let response: APIResponse<DoorbellStatus> = try await get(url: url)
        return try unwrap(response)
    }

    func testDoorbell() async throws {
        let url = baseURL.appendingPathComponent("doorbell/test")
        let _: APIResponse<TestResult> = try await post(url: url, body: nil)
    }

    // MARK: – Private helpers

    private func get<T: Decodable>(url: URL) async throws -> APIResponse<T> {
        var req = URLRequest(url: url)
        req.timeoutInterval = 10
        return try await execute(req)
    }

    private func put<T: Decodable>(url: URL, body: Data) async throws -> APIResponse<T> {
        var req = URLRequest(url: url)
        req.httpMethod = "PUT"
        req.setValue("application/json", forHTTPHeaderField: "Content-Type")
        req.httpBody = body
        req.timeoutInterval = 10
        return try await execute(req)
    }

    private func post<T: Decodable>(url: URL, body: Data?) async throws -> APIResponse<T> {
        var req = URLRequest(url: url)
        req.httpMethod = "POST"
        if let body {
            req.setValue("application/json", forHTTPHeaderField: "Content-Type")
            req.httpBody = body
        }
        req.timeoutInterval = 10
        return try await execute(req)
    }

    private func execute<T: Decodable>(_ request: URLRequest) async throws -> APIResponse<T> {
        let (data, response) = try await session.data(for: request)
        guard let http = response as? HTTPURLResponse else {
            throw CameraAPIError.networkError(URLError(.badServerResponse))
        }
        do {
            let decoded = try JSONDecoder().decode(APIResponse<T>.self, from: data)
            if !decoded.ok, let err = decoded.error {
                throw CameraAPIError.apiError(code: err.code, message: err.message)
            }
            return decoded
        } catch let e as CameraAPIError {
            throw e
        } catch {
            throw CameraAPIError.decodingError(error)
        }
    }

    private func unwrap<T>(_ response: APIResponse<T>) throws -> T {
        guard let data = response.data else {
            throw CameraAPIError.apiError(code: "MISSING_DATA", message: "Response missing data field")
        }
        return data
    }
}

// MARK: – Internal decodable helpers

private struct HealthData: Decodable {
    let status: String
    let firmwareVersion: String?
    let apiVersion: String?
}

struct StatusData: Decodable {
    let deviceId: String
    let hostname: String
    let firmwareVersion: String
    let apiVersion: String
    let uptime: Int
    let freeHeap: Int
    let rssi: Int
    let ipAddress: String
    let wifiConnected: Bool
    let streamClients: Int
}

private struct TestResult: Decodable {
    let triggered: Bool
}
