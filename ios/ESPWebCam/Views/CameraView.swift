// CameraView.swift – main live camera screen
import SwiftUI

struct CameraView: View {
    let device: CameraDevice

    @State private var viewModel = CameraViewModel()
    @State private var showSettings = false
    @State private var showShareSheet = false
    @State private var shareURL: URL?
    @Environment(\.scenePhase) private var scenePhase

    var body: some View {
        VStack(spacing: 0) {
            // ---- Status bar
            HStack {
                Circle()
                    .fill(statusColor)
                    .frame(width: 10, height: 10)
                Text(viewModel.connectionState.label)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                Spacer()
                if viewModel.isRecording {
                    RecordingIndicatorView(duration: viewModel.recordingDuration)
                }
            }
            .padding(.horizontal)
            .padding(.vertical, 6)

            // ---- Live video
            ZStack {
                Color.black
                if let img = viewModel.currentFrame {
                    Image(uiImage: img)
                        .resizable()
                        .scaledToFit()
                } else {
                    VStack(spacing: 8) {
                        ProgressView()
                            .tint(.white)
                        Text(viewModel.connectionState.label)
                            .foregroundStyle(.white)
                            .font(.caption)
                    }
                }
            }
            .frame(maxWidth: .infinity)
            .aspectRatio(4/3, contentMode: .fit)

            // ---- Controls
            HStack(spacing: 16) {
                // Record / Stop
                Button {
                    Task {
                        if viewModel.isRecording {
                            await viewModel.stopRecording()
                        } else {
                            viewModel.startRecording()
                        }
                    }
                } label: {
                    Label(
                        viewModel.isRecording ? "Stop" : "Record",
                        systemImage: viewModel.isRecording ? "stop.circle.fill" : "record.circle"
                    )
                    .foregroundStyle(viewModel.isRecording ? .red : .white)
                    .frame(maxWidth: .infinity)
                }
                .buttonStyle(.borderedProminent)
                .tint(viewModel.isRecording ? .red.opacity(0.2) : .blue)

                // Screenshot
                Button {
                    Task { await viewModel.takeScreenshot() }
                } label: {
                    Label("Screenshot", systemImage: "camera")
                        .frame(maxWidth: .infinity)
                }
                .buttonStyle(.bordered)

                // Settings
                Button {
                    showSettings = true
                } label: {
                    Image(systemName: "slider.horizontal.3")
                }
                .buttonStyle(.bordered)
            }
            .padding()

            // ---- Error messages
            if let err = viewModel.recordingError {
                Text(err).font(.caption).foregroundStyle(.red).padding(.horizontal)
            }
        }
        .navigationTitle(device.name)
        .navigationBarTitleDisplayMode(.inline)
        .onAppear {
            viewModel.connect(to: device)
        }
        .onDisappear {
            viewModel.disconnect()
        }
        .onChange(of: scenePhase) { _, phase in
            if phase == .background {
                viewModel.handleBackground()
            }
        }
        .sheet(isPresented: $showSettings) {
            CameraSettingsView(device: device)
        }
        .sheet(isPresented: $viewModel.showShareSheet) {
            if let url = viewModel.shareURL {
                ShareSheet(items: [url])
            }
        }
        .alert("🔔 Doorbell", isPresented: $viewModel.showDoorbellAlert) {
            Button("Dismiss", role: .cancel) {}
        } message: {
            if let event = viewModel.latestDoorbellEvent {
                Text("Doorbell pressed on \(event.deviceId) (event #\(event.sequence))")
            }
        }
    }

    private var statusColor: Color {
        switch viewModel.connectionState {
        case .connected:                  return .green
        case .connecting:                 return .orange
        case .idle, .disconnected:        return .gray
        case .failed:                     return .red
        }
    }
}

// MARK: – Recording indicator

struct RecordingIndicatorView: View {
    let duration: TimeInterval

    private var formatted: String {
        let s = Int(duration)
        return String(format: "%02d:%02d", s / 60, s % 60)
    }

    var body: some View {
        HStack(spacing: 4) {
            Circle().fill(.red).frame(width: 8, height: 8)
            Text("REC \(formatted)").font(.caption.monospacedDigit()).foregroundStyle(.red)
        }
    }
}

// MARK: – Share sheet

struct ShareSheet: UIViewControllerRepresentable {
    let items: [Any]
    func makeUIViewController(context: Context) -> UIActivityViewController {
        UIActivityViewController(activityItems: items, applicationActivities: nil)
    }
    func updateUIViewController(_ uiViewController: UIActivityViewController, context: Context) {}
}
