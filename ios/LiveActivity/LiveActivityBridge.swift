import ActivityKit
import Foundation

private struct RideSnapshot: Decodable {
    let speedMps: Double
    let distanceMeters: Double
    let distanceIncomplete: Bool
    let batteryLevel: Double
    let estimatedRangeMeters: Double
    let startedAt: Double
    let sampledAt: Double
    let connected: Bool
    let fresh: Bool
    let useImperial: Bool

    func validate() throws {
        guard estimatedRangeMeters.isFinite, estimatedRangeMeters >= -1,
              speedMps.isFinite, distanceMeters.isFinite, distanceMeters >= 0,
              batteryLevel.isFinite, batteryLevel >= -1, batteryLevel <= 1,
              startedAt.isFinite, startedAt > 0, sampledAt.isFinite, sampledAt >= 0 else {
            throw BridgeError.invalidSnapshot
        }
    }

    @available(iOS 16.2, *)
    var content: ActivityContent<RideActivityAttributes.ContentState> {
        let sampledDate = Date(timeIntervalSince1970: sampledAt)
        let state = RideActivityAttributes.ContentState(
            speedMps: abs(speedMps), distanceMeters: distanceMeters,
            distanceIncomplete: distanceIncomplete,
            batteryLevel: batteryLevel, estimatedRangeMeters: estimatedRangeMeters,
            connected: connected, fresh: fresh, useImperial: useImperial)
        // The system marks the view stale even if the phone stops executing.
        let expiry = connected && fresh ? sampledDate.addingTimeInterval(5) : Date()
        return ActivityContent(state: state, staleDate: expiry)
    }
}

private enum BridgeError: LocalizedError {
    case invalidSnapshot
    case alreadyActive
    case disabled

    var errorDescription: String? {
        switch self {
        case .invalidSnapshot: return "The ride data is invalid."
        case .alreadyActive: return "A ride Live Activity is already running."
        case .disabled: return "Enable Live Activities for VESC Tool in iPhone Settings."
        }
    }
}

// Qt invokes the bridge on its GUI thread. Keep defensive main-thread dispatch
// here so future callers cannot race state or issue Activity.request off-main.
private func onMain<T>(_ body: () -> T) -> T {
    if Thread.isMainThread { return body() }
    return DispatchQueue.main.sync(execute: body)
}

private var bridgeError = strdup("")!

private func setError(_ message: String) {
    let replacement = strdup(message)!
    free(bridgeError)
    bridgeError = replacement
}

@available(iOS 16.2, *)
private final class RideActivityController {
    static let shared = RideActivityController()
    private var activity: Activity<RideActivityAttributes>?
    private var pending: Task<Void, Never>?
    private var generation: UInt64 = 0
    private var latestContent: ActivityContent<RideActivityAttributes.ContentState>?
    private var pendingUpdateGeneration: UInt64?

    private init() {
        // A new phone process has no corresponding running Qt ride session.
        // Remove its old cards instead of leaving old speed visible indefinitely.
        let previousActivities = Activity<RideActivityAttributes>.activities
        if !previousActivities.isEmpty {
            enqueue {
                for previous in previousActivities {
                    await previous.end(nil, dismissalPolicy: .immediate)
                }
            }
        }
    }

    var isActive: Bool {
        guard let activity else { return false }
        return activity.activityState == .active || activity.activityState == .stale
    }

    private func enqueue(_ work: @escaping @MainActor () async -> Void) {
        let previous = pending
        pending = Task { @MainActor in
            await previous?.value
            await work()
        }
    }

    func start(_ snapshot: RideSnapshot) throws {
        guard !isActive else { throw BridgeError.alreadyActive }
        guard ActivityAuthorizationInfo().areActivitiesEnabled else { throw BridgeError.disabled }
        let attributes = RideActivityAttributes(startedAt: Date(timeIntervalSince1970: snapshot.startedAt))
        // Activity.request is synchronous: Qt only enters its active state after
        // the OS accepts the request, and can display the actual failure reason.
        let created = try Activity.request(attributes: attributes, content: snapshot.content, pushType: nil)
        generation &+= 1
        activity = created
    }

    func update(_ snapshot: RideSnapshot) {
        guard let current = activity, isActive else { return }
        latestContent = snapshot.content
        let expectedGeneration = generation
        // Keep only the newest pending reading if ActivityKit takes longer than
        // our sampling interval. Never replay a backlog of obsolete speeds.
        guard pendingUpdateGeneration != expectedGeneration else { return }
        pendingUpdateGeneration = expectedGeneration
        enqueue { [weak self] in
            guard let self else { return }
            defer {
                if self.pendingUpdateGeneration == expectedGeneration {
                    self.pendingUpdateGeneration = nil
                }
            }
            while self.generation == expectedGeneration,
                  self.activity?.id == current.id, self.isActive,
                  let content = self.latestContent {
                self.latestContent = nil
                await current.update(content)
            }
        }
    }

    func end() {
        guard let current = activity else { return }
        generation &+= 1
        activity = nil
        latestContent = nil
        pendingUpdateGeneration = nil
        // This runs after any in-flight update. Queued updates from the old
        // generation are skipped, including when a new ride starts immediately.
        enqueue {
            await current.end(nil, dismissalPolicy: .immediate)
        }
    }
}

private func decodeSnapshot(_ json: UnsafePointer<CChar>?) throws -> RideSnapshot {
    guard let json, let data = String(validatingCString: json)?.data(using: .utf8) else {
        throw BridgeError.invalidSnapshot
    }
    let snapshot = try JSONDecoder().decode(RideSnapshot.self, from: data)
    try snapshot.validate()
    return snapshot
}

@_cdecl("vesc_live_activity_supported")
public func vescLiveActivitySupported() -> Int32 {
    if #available(iOS 18.0, *) { return 1 }
    return 0
}

@_cdecl("vesc_live_activity_enabled")
public func vescLiveActivityEnabled() -> Int32 {
    onMain {
        if #available(iOS 18.0, *) {
            // Initialize cleanup at app launch, before a new ride is requested.
            _ = RideActivityController.shared
            return ActivityAuthorizationInfo().areActivitiesEnabled ? 1 : 0
        }
        return 0
    }
}

@_cdecl("vesc_live_activity_start")
public func vescLiveActivityStart(_ json: UnsafePointer<CChar>?) -> Int32 {
    onMain {
        do {
            if #available(iOS 18.0, *) {
                let snapshot = try decodeSnapshot(json)
                try RideActivityController.shared.start(snapshot)
                setError("")
                return 1
            }
            setError("Ride Live Activities require iOS 18 or later.")
        } catch {
            setError(error.localizedDescription)
        }
        return 0
    }
}

@_cdecl("vesc_live_activity_update")
public func vescLiveActivityUpdate(_ json: UnsafePointer<CChar>?) {
    onMain {
        do {
            if #available(iOS 18.0, *) {
                RideActivityController.shared.update(try decodeSnapshot(json))
            }
        } catch {
            setError(error.localizedDescription)
        }
    }
}

@_cdecl("vesc_live_activity_end")
public func vescLiveActivityEnd() {
    onMain {
        if #available(iOS 18.0, *) { RideActivityController.shared.end() }
    }
}

@_cdecl("vesc_live_activity_is_active")
public func vescLiveActivityIsActive() -> Int32 {
    onMain {
        if #available(iOS 18.0, *) { return RideActivityController.shared.isActive ? 1 : 0 }
        return 0
    }
}

@_cdecl("vesc_live_activity_last_error")
public func vescLiveActivityLastError() -> UnsafePointer<CChar> {
    onMain { UnsafePointer(bridgeError) }
}
