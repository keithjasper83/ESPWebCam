// ESPWebCamUITests.swift
import XCTest

final class ESPWebCamUITests: XCTestCase {
    var app: XCUIApplication!

    override func setUpWithError() throws {
        continueAfterFailure = false
        app = XCUIApplication()
        app.launch()
    }

    func testAppLaunchesWithoutCrash() {
        // Verify the app launches successfully
        XCTAssertTrue(app.state == .runningForeground)
    }

    func testRecordButtonNotActiveOnLaunch() {
        // The Record button must exist and not be mid-recording on launch
        // (Recording must never start automatically)
        let recordButton = app.buttons.matching(NSPredicate(format: "label CONTAINS 'Record'")).firstMatch
        XCTAssert(recordButton.exists)
        // If recording were active the label would show "Stop"
        XCTAssertFalse(app.buttons["Stop"].exists, "Recording must not be active on launch")
    }
}
