// ContentView.swift – root navigation
import SwiftUI

struct ContentView: View {
    @Environment(CameraStore.self) var cameraStore
    @State private var selectedDevice: CameraDevice?
    @State private var showAddCamera = false

    var body: some View {
        NavigationStack {
            if let device = selectedDevice ?? cameraStore.cameras.first {
                CameraView(device: device)
                    .navigationBarItems(trailing: Button("Cameras") {
                        showAddCamera = true
                    })
            } else {
                VStack(spacing: 20) {
                    Image(systemName: "camera.fill")
                        .font(.system(size: 60))
                        .foregroundStyle(.secondary)
                    Text("No cameras configured")
                        .font(.headline)
                    Button("Add Camera") { showAddCamera = true }
                        .buttonStyle(.borderedProminent)
                }
                .navigationTitle("ESPWebCam")
            }
        }
        .sheet(isPresented: $showAddCamera) {
            CameraDiscoveryView(selectedDevice: $selectedDevice)
        }
    }
}
