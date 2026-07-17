// MJPEGStreamClient.swift
// Connects to the ESP MJPEG endpoint and delivers parsed frames on the MainActor.
import Foundation

@MainActor
final class MJPEGStreamClient: NSObject {
    enum State {
        case idle, connecting, streaming, disconnected, failed(Error)
    }

    var onFrame: ((MJPEGFrame) -> Void)?
    var onStateChange: ((State) -> Void)?

    private(set) var state: State = .idle {
        didSet { onStateChange?(state) }
    }

    private let parser = MJPEGFrameParser()
    private var dataTask: URLSessionDataTask?
    private var urlSession: URLSession?
    private var streamURL: URL?

    func connect(to url: URL) {
        disconnect()
        streamURL = url

        parser.reset()
        parser.onFrame = { [weak self] frame in
            Task { @MainActor [weak self] in
                self?.onFrame?(frame)
            }
        }

        let config = URLSessionConfiguration.default
        config.timeoutIntervalForRequest = 30
        config.timeoutIntervalForResource = .infinity
        let session = URLSession(configuration: config, delegate: self, delegateQueue: nil)
        urlSession = session

        var request = URLRequest(url: url)
        request.timeoutInterval = .infinity
        let task = session.dataTask(with: request)
        dataTask = task

        state = .connecting
        task.resume()
    }

    func disconnect() {
        dataTask?.cancel()
        dataTask = nil
        urlSession?.invalidateAndCancel()
        urlSession = nil
        parser.reset()
        state = .idle
    }
}

// MARK: – URLSessionDataDelegate

extension MJPEGStreamClient: URLSessionDataDelegate {
    nonisolated func urlSession(_ session: URLSession,
                                dataTask: URLSessionDataTask,
                                didReceive response: URLResponse,
                                completionHandler: @escaping (URLSession.ResponseDisposition) -> Void) {
        guard let http = response as? HTTPURLResponse, http.statusCode == 200 else {
            Task { @MainActor [weak self] in
                self?.state = .failed(URLError(.badServerResponse))
            }
            completionHandler(.cancel)
            return
        }
        Task { @MainActor [weak self] in
            self?.state = .streaming
        }
        completionHandler(.allow)
    }

    nonisolated func urlSession(_ session: URLSession,
                                dataTask: URLSessionDataTask,
                                didReceive data: Data) {
        // Parsing happens off the main thread; frame delivery is dispatched back
        parser.append(data)
    }

    nonisolated func urlSession(_ session: URLSession,
                                task: URLSessionTask,
                                didCompleteWithError error: Error?) {
        Task { @MainActor [weak self] in
            if let error {
                // URLError.cancelled is expected on disconnect
                if (error as? URLError)?.code != .cancelled {
                    self?.state = .failed(error)
                }
            } else {
                self?.state = .disconnected
            }
        }
    }
}
