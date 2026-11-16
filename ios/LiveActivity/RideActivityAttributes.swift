import ActivityKit
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
    }

    public var startedAt: Date
}
