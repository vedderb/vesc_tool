import ActivityKit
import SwiftUI
import WidgetKit
import UIKit

enum RidePalette {
    static let background = Color(red: 39.0 / 255, green: 39.0 / 255, blue: 39.0 / 255)
    static let text = Color(red: 220.0 / 255, green: 220.0 / 255, blue: 220.0 / 255)
    static let secondary = Color(red: 180.0 / 255, green: 180.0 / 255, blue: 180.0 / 255)
    static let accent = Color(red: 129.0 / 255, green: 212.0 / 255, blue: 250.0 / 255)
}

@main
struct VESCRideWidgetBundle: WidgetBundle {
    var body: some Widget {
        RideActivityWidget()
    }
}

struct RideActivityWidget: Widget {
    var body: some WidgetConfiguration {
        rideActivityConfiguration().supplementalActivityFamilies([.small])
    }
}

private func rideActivityConfiguration() -> ActivityConfiguration<RideActivityAttributes> {
    ActivityConfiguration(for: RideActivityAttributes.self) { context in
        RideActivityRoot(context: context)
            .activityBackgroundTint(RidePalette.background)
            .activitySystemActionForegroundColor(.white)
    } dynamicIsland: { context in
        let values = RideDisplayValues(context)
        return DynamicIsland {
            DynamicIslandExpandedRegion(.leading) {
                VStack(alignment: .leading, spacing: 1) {
                    Text(values.speed)
                        .font(.custom("Roboto-Regular", fixedSize: values.estimatedRange == nil ? 34 : 30))
                        .monospacedDigit()
                        .foregroundStyle(values.isCurrent ? RidePalette.text : RidePalette.secondary)
                    Text(values.speedUnit.uppercased())
                        .font(.custom("Roboto-Regular", fixedSize: 11)).foregroundStyle(RidePalette.secondary)
                }
            }
            DynamicIslandExpandedRegion(.trailing) {
                VStack(alignment: .trailing, spacing: 3) {
                    Text("Trip: " + values.distance + " " + values.distanceUnit)
                        .accessibilityLabel(values.distanceAccessibility)
                        .font(.headline).monospacedDigit()
                    BatteryValue(values: values).font(.caption)
                    EstimatedRangeValue(values: values).font(.caption2)
                }
            }
            DynamicIslandExpandedRegion(.bottom) {
                HStack {
                    if !values.isCurrent { RideStatus(values: values) }
                    Spacer()
                    RideTimer(startedAt: context.attributes.startedAt)
                }
                .font(.caption)
            }
        } compactLeading: {
            HStack(spacing: 2) {
                VESCMark(size: 13)
                Text(values.speed).font(.custom("Roboto-Regular", fixedSize: 15)).monospacedDigit()
            }
            .foregroundStyle(values.isCurrent ? RidePalette.text : RidePalette.secondary)
            .accessibilityLabel(values.speedAccessibility)
        } compactTrailing: {
            Text(values.speedUnit.uppercased()).font(.custom("Roboto-Regular", fixedSize: 9))
        } minimal: {
            Text(values.speed).font(.custom("Roboto-Regular", fixedSize: 15)).monospacedDigit()
                .foregroundStyle(values.isCurrent ? RidePalette.text : RidePalette.secondary)
                .accessibilityLabel(values.speedAccessibility)
        }
        .keylineTint(RidePalette.accent)
    }
}

private struct RideActivityRoot: View {
    @Environment(\.activityFamily) private var activityFamily
    let context: ActivityViewContext<RideActivityAttributes>

    var body: some View {
        switch activityFamily {
        case .small:
            RideWatchView(values: RideDisplayValues(context))
        default:
            RideLockScreenView(values: RideDisplayValues(context), startedAt: context.attributes.startedAt)
        }
    }
}

struct RideLockScreenView: View {
    let values: RideDisplayValues
    let startedAt: Date

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                VESCMark(size: 19)
                Spacer()
                RideTimer(startedAt: startedAt).font(.caption)
            }
            HStack(alignment: .firstTextBaseline, spacing: 18) {
                HStack(alignment: .firstTextBaseline, spacing: 5) {
                    Text(values.speed)
                        .font(.custom("Roboto-Regular", fixedSize: 46))
                        .monospacedDigit()
                        .foregroundStyle(values.isCurrent ? RidePalette.text : RidePalette.secondary)
                    Text(values.speedUnit.uppercased())
                        .font(.custom("Roboto-Regular", fixedSize: 13)).foregroundStyle(RidePalette.secondary)
                }
                .accessibilityElement(children: .ignore)
                .accessibilityLabel(values.speedAccessibility)
                Spacer(minLength: 0)
                VStack(alignment: .trailing, spacing: 3) {
                    Text("Trip: " + values.distance + " " + values.distanceUnit)
                        .accessibilityLabel(values.distanceAccessibility)
                        .font(.title3.weight(.semibold)).monospacedDigit()
                    BatteryValue(values: values).font(.caption).foregroundStyle(RidePalette.secondary)
                    EstimatedRangeValue(values: values).font(.caption)
                }
            }
            if !values.isCurrent {
                RideStatus(values: values).font(.caption)
            }
        }
        .lineLimit(1)
        .minimumScaleFactor(0.7)
        .padding(16)
        .foregroundStyle(RidePalette.text)
    }
}

struct RideWatchView: View {
    @Environment(\.isLuminanceReduced) private var isLuminanceReduced
    let values: RideDisplayValues

    var body: some View {
        VStack(alignment: .leading, spacing: 3) {
            HStack(alignment: .firstTextBaseline, spacing: 4) {
                Text(values.speed)
                    .font(.custom("Roboto-Regular", fixedSize: values.estimatedRange == nil ? 34 : 30))
                    .monospacedDigit()
                    .foregroundStyle(values.isCurrent ? RidePalette.text : RidePalette.secondary)
                Text(values.speedUnit.uppercased())
                    .font(.custom("Roboto-Regular", fixedSize: 10)).foregroundStyle(RidePalette.secondary)
                Spacer(minLength: 0)
                VESCMark(size: 18).opacity(isLuminanceReduced ? 0.65 : 1)
            }
            .accessibilityElement(children: .ignore)
            .accessibilityLabel(values.speedAccessibility)
            HStack(spacing: 4) {
                Text("Trip: " + values.distance + " " + values.distanceUnit).monospacedDigit()
                    .accessibilityLabel(values.distanceAccessibility)
                Spacer(minLength: 2)
                BatteryValue(values: values).layoutPriority(1)
            }
            .font(.custom("Roboto-Regular", fixedSize: 11))
            .minimumScaleFactor(0.7)
            EstimatedRangeValue(values: values, compact: true)
                .font(.custom("Roboto-Regular", fixedSize: 11))
            if !values.isCurrent {
                RideStatus(values: values).font(.caption2)
            }
        }
        .lineLimit(1)
        .minimumScaleFactor(0.7)
        .padding(.horizontal, 10)
        .padding(.vertical, 6)
        .foregroundStyle(RidePalette.text)
    }
}

struct RideDisplayValues {
    let isCurrent: Bool
    let speed: String
    let speedUnit: String
    let distance: String
    let distanceUnit: String
    let distanceIncomplete: Bool
    let battery: String
    let batterySymbol: String
    let estimatedRange: String?
    let status: String

    init(_ context: ActivityViewContext<RideActivityAttributes>) {
        let state = context.state
        isCurrent = state.connected && state.fresh && !context.isStale
        speedUnit = state.useImperial ? "mph" : "km/h"
        distanceUnit = state.useImperial ? "mi" : "km"
        speed = isCurrent ? String(format: "%.0f", abs(state.speedMps) * (state.useImperial ? 2.2369362921 : 3.6)) : "—"
        distanceIncomplete = state.distanceIncomplete
        let convertedDistance = state.distanceMeters / (state.useImperial ? 1609.344 : 1000)
        let shownDistance = distanceIncomplete ? floor(convertedDistance * 10) / 10 : convertedDistance
        distance = String(format: "%.1f", shownDistance) + (distanceIncomplete ? "+" : "")
        if isCurrent && state.batteryLevel >= 0 && state.batteryLevel <= 1 {
            let percent = Int((state.batteryLevel * 100).rounded())
            battery = "\(percent)%"
            if percent <= 10 { batterySymbol = "battery.0percent" }
            else if percent <= 35 { batterySymbol = "battery.25percent" }
            else if percent <= 60 { batterySymbol = "battery.50percent" }
            else if percent <= 85 { batterySymbol = "battery.75percent" }
            else { batterySymbol = "battery.100percent" }
        } else {
            battery = "—"
            batterySymbol = "battery.0percent"
        }
        if isCurrent && state.estimatedRangeMeters.isFinite && state.estimatedRangeMeters >= 0 {
            let remaining = state.estimatedRangeMeters / (state.useImperial ? 1609.344 : 1000)
            let format = remaining < 10 ? "%.1f" : "%.0f"
            estimatedRange = String(format: format, remaining) + " " + distanceUnit
        } else {
            estimatedRange = nil
        }
        if !state.connected { status = "Disconnected" }
        else if !isCurrent { status = "No data" }
        else { status = "Live" }
    }

    var distanceAccessibility: String {
        "Trip distance " + (distanceIncomplete ? "at least " : "") + distance.replacingOccurrences(of: "+", with: "") + " " + distanceUnit
    }

    var speedAccessibility: String {
        isCurrent ? "Speed \(speed) \(speedUnit)" : "Speed unavailable, \(status.lowercased())"
    }
}

private struct VESCMark: View {
    let size: CGFloat

    var body: some View {
        if let icon = UIImage(named: "v_icon-256") {
            Image(uiImage: icon)
                .renderingMode(.template)
                .resizable()
                .scaledToFit()
                .frame(width: size, height: size)
                .foregroundStyle(RidePalette.text)
                .accessibilityHidden(true)
        }
    }
}

private struct RideStatus: View {
    let values: RideDisplayValues

    var body: some View {
        Label(values.status, systemImage: "antenna.radiowaves.left.and.right.slash")
            .foregroundStyle(RidePalette.secondary)
    }
}

private struct BatteryValue: View {
    let values: RideDisplayValues

    var body: some View {
        Label(values.battery, systemImage: values.batterySymbol)
            .monospacedDigit()
            .accessibilityLabel("Battery \(values.battery)")
    }
}

private struct EstimatedRangeValue: View {
    let values: RideDisplayValues
    var compact = false

    var body: some View {
        if let range = values.estimatedRange {
            Text(compact ? "Est. \(range) left" : "Est. range \(range)")
                .monospacedDigit()
                .foregroundStyle(RidePalette.secondary)
                .accessibilityLabel("Estimated remaining range \(range)")
        }
    }
}

private struct RideTimer: View {
    let startedAt: Date

    var body: some View {
        Text(timerInterval: startedAt...Date.distantFuture, countsDown: false)
            .monospacedDigit()
            .multilineTextAlignment(.trailing)
            .accessibilityLabel("Ride elapsed time")
    }
}
