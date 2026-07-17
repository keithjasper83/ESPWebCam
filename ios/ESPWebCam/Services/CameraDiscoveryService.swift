// CameraDiscoveryService.swift
// Discovers ESPWebCam devices on the local network using Network.framework Bonjour.
import Foundation
import Network

struct DiscoveredCamera {
    let name: String
    let host: String
    let port: UInt16
}

@MainActor
final class CameraDiscoveryService {
    var onDiscovered: ((DiscoveredCamera) -> Void)?
    var onLost: ((String) -> Void)?   // name of removed service

    private var browser: NWBrowser?

    func startDiscovery() {
        let params = NWParameters()
        params.includePeerToPeer = true
        let descriptor = NWBrowser.Descriptor.bonjour(type: "_espwebcam-stream._tcp", domain: nil)
        let browser    = NWBrowser(for: descriptor, using: params)

        browser.stateUpdateHandler = { [weak self] state in
            Task { @MainActor [weak self] in
                switch state {
                case .failed(let err):
                    print("[CameraDiscovery] Browser failed: \(err)")
                default:
                    break
                }
            }
        }

        browser.browseResultsChangedHandler = { [weak self] results, changes in
            Task { @MainActor [weak self] in
                for change in changes {
                    switch change {
                    case .added(let result):
                        self?.handleAdded(result)
                    case .removed(let result):
                        if case .service(let name, _, _, _) = result.endpoint {
                            self?.onLost?(name)
                        }
                    default:
                        break
                    }
                }
            }
        }

        browser.start(queue: .main)
        self.browser = browser
    }

    func stopDiscovery() {
        browser?.cancel()
        browser = nil
    }

    private func handleAdded(_ result: NWBrowser.Result) {
        guard case .service(let name, _, _, _) = result.endpoint else { return }

        // Resolve the endpoint to a hostname/port
        let connection = NWConnection(to: result.endpoint, using: .tcp)
        connection.stateUpdateHandler = { [weak self] state in
            if case .ready = state {
                if let remote = connection.currentPath?.remoteEndpoint,
                   case .hostPort(let host, let port) = remote {
                    let hostStr: String
                    switch host {
                    case .name(let n, _): hostStr = n
                    case .ipv4(let a):    hostStr = "\(a)"
                    case .ipv6(let a):    hostStr = "\(a)"
                    @unknown default:     hostStr = name
                    }
                    let discovered = DiscoveredCamera(
                        name: name, host: hostStr, port: port.rawValue)
                    Task { @MainActor [weak self] in
                        self?.onDiscovered?(discovered)
                    }
                }
                connection.cancel()
            } else if case .failed = state {
                connection.cancel()
            }
        }
        connection.start(queue: .main)
    }
}
