// CameraSettingsView.swift
import SwiftUI

struct CameraSettingsView: View {
    let device: CameraDevice
    @State private var vm = CameraSettingsViewModel()
    @Environment(\.dismiss) private var dismiss

    private let frameSizeOptions: [(label: String, value: Int)] = [
        ("QVGA (320×240)", 5),
        ("HVGA (480×320)", 6),
        ("VGA (640×480)",  8),
        ("SVGA (800×600)", 9),
        ("XGA (1024×768)", 10),
    ]

    var body: some View {
        NavigationStack {
            Form {
                Section("Resolution") {
                    Picker("Frame Size", selection: $vm.settings.frameSize) {
                        ForEach(frameSizeOptions, id: \.value) { opt in
                            Text(opt.label).tag(opt.value)
                        }
                    }
                }

                Section("Quality") {
                    LabeledContent("JPEG Quality: \(vm.settings.jpegQuality)") {
                        Slider(value: Binding(
                            get: { Double(vm.settings.jpegQuality) },
                            set: { vm.settings.jpegQuality = Int($0) }
                        ), in: 4...63, step: 1)
                    }
                    Text("4 = highest quality  ·  63 = smallest file")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                Section("Image Adjustments") {
                    IntSlider(label: "Brightness", value: $vm.settings.brightness, range: -2...2)
                    IntSlider(label: "Contrast",   value: $vm.settings.contrast,   range: -2...2)
                    IntSlider(label: "Saturation", value: $vm.settings.saturation, range: -2...2)
                    IntSlider(label: "Exposure",   value: $vm.settings.aeLevel,    range: -5...5)
                }

                Section("Orientation") {
                    Toggle("Horizontal Mirror", isOn: Binding(
                        get: { vm.settings.hmirror != 0 },
                        set: { vm.settings.hmirror = $0 ? 1 : 0 }
                    ))
                    Toggle("Vertical Flip", isOn: Binding(
                        get: { vm.settings.vflip != 0 },
                        set: { vm.settings.vflip = $0 ? 1 : 0 }
                    ))
                }

                Section("Sensor") {
                    LabeledContent("Sensor PID", value: vm.settings.sensorPid)
                    LabeledContent("Frame Size", value: vm.settings.frameSizeName)
                }

                if let err = vm.errorMessage {
                    Section { Text(err).foregroundStyle(.red) }
                }
                if let msg = vm.successMessage {
                    Section { Text(msg).foregroundStyle(.green) }
                }
            }
            .navigationTitle("Camera Settings")
            .navigationBarItems(
                leading: Button("Cancel") { dismiss() },
                trailing: Button("Apply") {
                    Task { await vm.save() }
                }
                .disabled(vm.isSaving)
            )
            .task { vm.configure(device: device); await vm.load() }
        }
    }
}

// MARK: – Helper: integer slider

private struct IntSlider: View {
    let label: String
    @Binding var value: Int
    let range: ClosedRange<Int>

    var body: some View {
        LabeledContent("\(label): \(value)") {
            Slider(value: Binding(
                get: { Double(value) },
                set: { value = Int($0.rounded()) }
            ), in: Double(range.lowerBound)...Double(range.upperBound), step: 1)
        }
    }
}
