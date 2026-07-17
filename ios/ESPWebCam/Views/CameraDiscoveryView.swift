// CameraDiscoveryView.swift – add/select camera
import SwiftUI

struct CameraDiscoveryView: View {
    @Binding var selectedDevice: CameraDevice?
    @Environment(CameraStore.self) var cameraStore
    @Environment(\.dismiss) private var dismiss

    @State private var discoveryService = CameraDiscoveryService()
    @State private var discovered: [DiscoveredCamera] = []
    @State private var manualHost = ""
    @State private var manualName = ""
    @State private var showManual = false

    var body: some View {
        NavigationStack {
            List {
                Section("Discovered on Network") {
                    if discovered.isEmpty {
                        HStack {
                            ProgressView()
                            Text("Searching for cameras…").foregroundStyle(.secondary)
                        }
                    } else {
                        ForEach(discovered, id: \.host) { cam in
                            Button {
                                let device = CameraDevice(name: cam.name, host: cam.host,
                                                          streamPort: cam.port)
                                cameraStore.add(device)
                                selectedDevice = device
                                dismiss()
                            } label: {
                                VStack(alignment: .leading) {
                                    Text(cam.name).font(.headline)
                                    Text(cam.host).font(.caption).foregroundStyle(.secondary)
                                }
                            }
                            .buttonStyle(.plain)
                        }
                    }
                }

                Section("Saved Cameras") {
                    ForEach(cameraStore.cameras) { cam in
                        Button {
                            selectedDevice = cam
                            dismiss()
                        } label: {
                            VStack(alignment: .leading) {
                                Text(cam.name).font(.headline)
                                Text(cam.host).font(.caption).foregroundStyle(.secondary)
                            }
                        }
                        .buttonStyle(.plain)
                    }
                    .onDelete { idx in
                        idx.forEach { cameraStore.remove(cameraStore.cameras[$0]) }
                    }
                }

                Section {
                    Button("Add Manually") { showManual = true }
                }
            }
            .navigationTitle("Cameras")
            .navigationBarItems(trailing: Button("Done") { dismiss() })
            .onAppear { discoveryService.startDiscovery() }
            .onDisappear { discoveryService.stopDiscovery() }
            .onReceive(discoveryPublisher) { cam in
                if !discovered.contains(where: { $0.host == cam.host }) {
                    discovered.append(cam)
                }
            }
        }
        .sheet(isPresented: $showManual) {
            ManualAddView { name, host in
                let device = CameraDevice(name: name, host: host)
                cameraStore.add(device)
                selectedDevice = device
                dismiss()
            }
        }
    }

    // Bridge Combine-less callback to a publisher using a PassthroughSubject equivalent
    private var discoveryPublisher: NotificationCenter.Publisher {
        // We use NotificationCenter as a simple bridge for the callback
        NotificationCenter.default.publisher(for: .cameraDiscovered)
    }
}

extension Notification.Name {
    static let cameraDiscovered = Notification.Name("cameraDiscovered")
}

// MARK: – Manual add sheet

private struct ManualAddView: View {
    let onAdd: (String, String) -> Void
    @Environment(\.dismiss) private var dismiss
    @State private var name = "ESP Camera"
    @State private var host = "espwebcam.local"

    var body: some View {
        NavigationStack {
            Form {
                Section("Camera Details") {
                    TextField("Name", text: $name)
                    TextField("Hostname or IP", text: $host)
                        .autocorrectionDisabled()
                        .textInputAutocapitalization(.never)
                        .keyboardType(.URL)
                }
            }
            .navigationTitle("Add Camera")
            .navigationBarItems(
                leading: Button("Cancel") { dismiss() },
                trailing: Button("Add") {
                    guard !name.isEmpty, !host.isEmpty else { return }
                    onAdd(name, host)
                    dismiss()
                }
            )
        }
    }
}
