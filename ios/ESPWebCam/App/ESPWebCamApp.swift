// ESPWebCamApp.swift – @main entry point
import SwiftUI

@main
struct ESPWebCamApp: App {
    @State private var cameraStore = CameraStore()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environment(cameraStore)
        }
    }
}
