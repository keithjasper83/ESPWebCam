// DoorbellEvent.swift – doorbell event model
import Foundation

/// A doorbell press event as returned by GET /api/v1/doorbell/status.
/// Field names match the OpenAPI schema exactly.
struct DoorbellEvent: Decodable, Equatable {
    let eventType: String
    let eventId: String
    let deviceId: String
    let occurredAtMs: UInt32
    let sequence: UInt32
}

struct DoorbellStatus: Decodable {
    let totalEvents: Int
    let lastEvent: DoorbellEvent?
}
