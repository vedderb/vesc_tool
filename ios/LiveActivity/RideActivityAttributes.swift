import ActivityKit
import AppIntents
import Foundation

// This type is compiled into both the phone bridge and the widget extension.
@available(iOS 16.2, *)
public struct RideActivityAttributes: ActivityAttributes {
    public struct ContentState: Codable, Hashable {
        public var speedMps: Double
        public var distanceMeters: Double
        public var distanceIncomplete: Bool
        public var batteryLevel: Double
        public var estimatedRangeMeters: Double
        public var connected: Bool
        public var fresh: Bool
        public var useImperial: Bool
        public var logging: Bool
    }

    public var startedAt: Date
}

// The logging button of the Live Activity. Performed in the phone app, where RideSession polls for it.
@available(iOS 17.0, *)
public struct RideLogToggleIntent: LiveActivityIntent {
    public static var title: LocalizedStringResource = "Toggle RT Data Logging"
    public static var isDiscoverable = false
    static var pending = false

    public init() {}

    @MainActor
    public func perform() async throws -> some IntentResult {
        RideLogToggleIntent.pending = true
        return .result()
    }
}
