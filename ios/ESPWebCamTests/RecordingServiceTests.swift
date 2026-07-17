// RecordingServiceTests.swift
// Verifies recording state machine rules.
import XCTest
@testable import ESPWebCam

@MainActor
final class RecordingServiceTests: XCTestCase {

    func testInitialStateIsNotRecording() {
        let svc = RecordingService()
        XCTAssertFalse(svc.isRecording)
        XCTAssertNil(svc.startTime)
        XCTAssertEqual(svc.duration, 0)
    }

    func testStartRecordingSetsIsRecording() throws {
        let svc = RecordingService()
        try svc.startRecording()
        XCTAssertTrue(svc.isRecording)
        XCTAssertNotNil(svc.startTime)
    }

    func testDoubleStartThrows() throws {
        let svc = RecordingService()
        try svc.startRecording()
        XCTAssertThrowsError(try svc.startRecording()) { error in
            XCTAssert(error is RecordingError)
        }
    }

    func testStopWithoutStartThrows() async {
        let svc = RecordingService()
        do {
            _ = try await svc.stopRecording()
            XCTFail("Expected RecordingError.notRecording")
        } catch let e as RecordingError {
            if case .notRecording = e { /* expected */ } else {
                XCTFail("Wrong error: \(e)")
            }
        } catch {
            XCTFail("Wrong error type: \(error)")
        }
    }

    func testStartThenStopResetsState() async throws {
        let svc = RecordingService()
        try svc.startRecording()
        XCTAssertTrue(svc.isRecording)
        _ = try await svc.stopRecording()
        XCTAssertFalse(svc.isRecording)
        XCTAssertNil(svc.startTime)
    }
}
