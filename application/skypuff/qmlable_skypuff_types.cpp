#include "qmlable_skypuff_types.h"

bool QMLable_skypuff_config::deserializeV1(VByteArray& from)
{
    motor_poles = from.vbPopFrontUint8();
    gear_ratio = from.vbPopFrontDouble32Auto();
    wheel_diameter = from.vbPopFrontDouble32Auto();
    battery_type = (int)from.vbPopFrontUint8();
    battery_cells = (int)from.vbPopFrontUint8();

    amps_per_kg = from.vbPopFrontDouble16(1e2);
    pull_applying_period = from.vbPopFrontUint16();
    rope_length = from.vbPopFrontInt32();
    braking_length = from.vbPopFrontInt32();
    braking_extension_length = from.vbPopFrontInt32();

    slowing_length = from.vbPopFrontInt32();
    slow_erpm = from.vbPopFrontUint16();
    rewinding_trigger_length = from.vbPopFrontInt32();
    unwinding_trigger_length = from.vbPopFrontInt32();
    pull_current = from.vbPopFrontDouble16(10);

    pre_pull_k = from.vbPopFrontDouble16(1e4);
    takeoff_pull_k = from.vbPopFrontDouble16(1e4);
    fast_pull_k = from.vbPopFrontDouble16(1e4);
    takeoff_trigger_length = from.vbPopFrontUint16();
    pre_pull_timeout = from.vbPopFrontUint16();

    takeoff_period = from.vbPopFrontUint16();
    brake_current = from.vbPopFrontDouble16(10);
    slowing_current = from.vbPopFrontDouble16(10);
    manual_brake_current = from.vbPopFrontDouble16(10);
    unwinding_current = from.vbPopFrontDouble16(10);

    rewinding_current = from.vbPopFrontDouble16(10);
    slow_max_current = from.vbPopFrontDouble16(10);
    manual_slow_max_current = from.vbPopFrontDouble16(10);
    manual_slow_speed_up_current = from.vbPopFrontDouble16(10);
    manual_slow_erpm = from.vbPopFrontUint16();

    antisex_min_pull_amps = from.vbPopFrontDouble16(10);
    antisex_reduce_amps = from.vbPopFrontDouble16(10);
    antisex_acceleration_on_mss = from.vbPopFrontDouble16(1e2);
    antisex_acceleration_off_mss = from.vbPopFrontDouble16(1e2);
    antisex_max_period_ms = (int)from.vbPopFrontUint16();

    max_speed_ms = from.vbPopFrontDouble16(1e2);
    braking_applying_period = (int)from.vbPopFrontUint16();
    unwinding_strong_current = from.vbPopFrontDouble16(10);
    unwinding_strong_erpm = (int)from.vbPopFrontInt16();

    return true;
}

QByteArray QMLable_skypuff_config::serializeV1() const
{
    VByteArray vb;

    vb.vbAppendUint8(SK_COMM_SETTINGS_V1); // Version

    vb.vbAppendUint8(motor_poles);
    vb.vbAppendDouble32Auto(gear_ratio);
    vb.vbAppendDouble32Auto(wheel_diameter);
    vb.vbAppendUint8(battery_type);
    vb.vbAppendUint8(battery_cells);

    vb.vbAppendDouble16(amps_per_kg, 1e2);
    vb.vbAppendUint16(pull_applying_period);
    vb.vbAppendInt32(rope_length);
    vb.vbAppendInt32(braking_length);
    vb.vbAppendInt32(braking_extension_length);

    vb.vbAppendInt32(slowing_length);
    vb.vbAppendUint16(slow_erpm);
    vb.vbAppendInt32(rewinding_trigger_length);
    vb.vbAppendInt32(unwinding_trigger_length);
    vb.vbAppendDouble16(pull_current, 10);

    vb.vbAppendDouble16(pre_pull_k, 1e4);
    vb.vbAppendDouble16(takeoff_pull_k, 1e4);
    vb.vbAppendDouble16(fast_pull_k, 1e4);
    vb.vbAppendUint16(takeoff_trigger_length);
    vb.vbAppendUint16(pre_pull_timeout);

    vb.vbAppendUint16(takeoff_period);
    vb.vbAppendDouble16(brake_current, 10);
    vb.vbAppendDouble16(slowing_current, 10);
    vb.vbAppendDouble16(manual_brake_current, 10);
    vb.vbAppendDouble16(unwinding_current, 10);

    vb.vbAppendDouble16(rewinding_current, 10);
    vb.vbAppendDouble16(slow_max_current, 10);
    vb.vbAppendDouble16(manual_slow_max_current, 10);
    vb.vbAppendDouble16(manual_slow_speed_up_current, 10);
    vb.vbAppendUint16(manual_slow_erpm);

    vb.vbAppendDouble16(antisex_min_pull_amps, 10);
    vb.vbAppendDouble16(antisex_reduce_amps, 10);
    vb.vbAppendDouble16(antisex_acceleration_on_mss, 1e2);
    vb.vbAppendDouble16(antisex_acceleration_off_mss, 1e2);
    vb.vbAppendUint16(antisex_max_period_ms);

    vb.vbAppendDouble16(max_speed_ms, 1e2);
    vb.vbAppendUint16(braking_applying_period);
    vb.vbAppendDouble16(unwinding_strong_current, 10);
    vb.vbAppendInt16(unwinding_strong_erpm);

    return std::move(vb);
}

bool QMLable_skypuff_config::saveV1(QSettings &f) const
{
    f.beginGroup("settings");
    f.setValue("version", 1);
    f.endGroup();

    f.beginGroup("vesc_drive");
    f.setValue("motor_poles", motor_poles);
    f.setValue("gear_ratio", QString::number(gear_ratio, 'f', 6));
    f.setValue("wheel_diameter_mm", wheel_diameter_to_mm());
    f.endGroup();

    f.beginGroup("skypuff_drive");
    f.setValue("amps_per_kg", QString::number(amps_per_kg, 'f', 3));
    f.setValue("pull_applying_period", QString::number(pull_applying_period_to_seconds(), 'f', 1));
    f.setValue("braking_applying_period", QString::number(braking_applying_period_to_seconds(), 'f', 1));
    f.setValue("rope_length", QString::number(rope_length_to_meters(), 'f', 1));
    f.setValue("max_speed_ms", QString::number(max_speed_ms, 'f', 1));
    f.setValue("battery_cells", battery_cells);
    f.setValue("battery_type", battery_type);
    f.endGroup();

    f.beginGroup("braking_zone");
    f.setValue("braking_length", QString::number(braking_length_to_meters(), 'f', 1));
    f.setValue("braking_extension_length", QString::number(braking_extension_length_to_meters(), 'f', 1));
    f.setValue("brake_kg", QString::number(brake_current_to_kg(), 'f', 1));
    f.endGroup();

    f.beginGroup("unwinding");
    f.setValue("unwinding_trigger_length", QString::number(unwinding_trigger_length_to_meters(), 'f', 1));
    f.setValue("unwinding_kg", QString::number(unwinding_current_to_kg(), 'f', 1));
    f.setValue("unwinding_strong_kg", QString::number(unwinding_strong_current_to_kg(), 'f', 1));
    f.setValue("unwinding_strong_ms", QString::number(unwinding_strong_erpm_to_ms(), 'f', 1));
    f.endGroup();

    f.beginGroup("rewinding");
    f.setValue("rewinding_trigger_length", QString::number(rewinding_trigger_length_to_meters(), 'f', 1));
    f.setValue("rewinding_kg", QString::number(rewinding_current_to_kg(), 'f', 1));
    f.endGroup();

    f.beginGroup("slowing");
    f.setValue("slowing_length", QString::number(slowing_length_to_meters(), 'f', 1));
    f.setValue("slowing_kg", QString::number(slowing_current_to_kg(), 'f', 1));
    f.setValue("slow_ms", QString::number(slow_erpm_to_ms(), 'f', 1));
    f.setValue("slow_max_kg", QString::number(slow_max_current_to_kg(), 'f', 1));
    f.endGroup();

    f.beginGroup("winch");
    f.setValue("pull_kg", QString::number(pull_current_to_kg(),'f',1));
    f.setValue("pre_pull_k", pre_pull_k_to_percents());
    f.setValue("takeoff_pull_k", takeoff_pull_k_to_percents());
    f.setValue("fast_pull_k", fast_pull_k_to_percents());
    f.setValue("pre_pull_timeout", QString::number(pre_pull_timeout_to_seconds(), 'f', 1));
    f.setValue("takeoff_trigger_length", QString::number(takeoff_trigger_length_to_meters(), 'f', 1));
    f.setValue("takeoff_period", QString::number(takeoff_period_to_seconds(), 'f', 1));
    f.endGroup();

    f.beginGroup("manual");
    f.setValue("manual_brake_kg", QString::number(manual_brake_current_to_kg(), 'f', 1));
    f.setValue("manual_slow_ms", QString::number(manual_slow_erpm_to_ms(), 'f', 1));
    f.setValue("manual_slow_speed_up_kg", QString::number(manual_slow_speed_up_current_to_kg(), 'f', 1));
    f.setValue("manual_slow_max_kg", QString::number(manual_slow_max_current_to_kg(), 'f', 1));
    f.endGroup();

    f.beginGroup("antisex");
    f.setValue("min_pull_kg", QString::number(antisex_min_pull_amps_to_kg(), 'f', 1));
    f.setValue("reduce_kg", QString::number(antisex_reduce_amps_to_kg(), 'f', 1));
    f.setValue("acceleration_on_mss", QString::number(antisex_acceleration_on_mss, 'f', 1));
    f.setValue("acceleration_off_mss", QString::number(antisex_acceleration_off_mss, 'f', 1));
    f.setValue("max_period_secs", QString::number(antisex_max_period_to_seconds(), 'f', 1));
    f.endGroup();

    f.sync();
    return f.status() == QSettings::NoError;
}

bool QMLable_skypuff_config::loadV1(QSettings &f)
{
    f.beginGroup("vesc_drive");
    motor_poles = f.value("motor_poles").toInt();
    gear_ratio = f.value("gear_ratio").toDouble();
    wheel_diameter_from_mm(f.value("wheel_diameter_mm").toInt());
    f.endGroup();

    f.beginGroup("skypuff_drive");
    amps_per_kg = f.value("amps_per_kg").toDouble();
    seconds_to_pull_applying_period(f.value("pull_applying_period").toDouble());
    seconds_to_braking_applying_period(f.value("braking_applying_period").toDouble());
    meters_to_rope_length(f.value("rope_length").toDouble());
    max_speed_ms =  f.value("max_speed_ms").toDouble();
    battery_cells = f.value("battery_cells").toInt();
    battery_type = f.value("battery_type").toInt();
    f.endGroup();

    f.beginGroup("braking_zone");
    meters_to_braking_length(f.value("braking_length").toDouble());
    meters_to_braking_extension_length(f.value("braking_extension_length").toDouble());
    kg_to_brake_current( f.value("brake_kg").toDouble());
    f.endGroup();

    f.beginGroup("unwinding");
    meters_to_unwinding_trigger_length(f.value("unwinding_trigger_length").toDouble());
    kg_to_unwinding_current(f.value("unwinding_kg").toDouble());
    kg_to_unwinding_strong_current(f.value("unwinding_strong_kg").toDouble());
    ms_to_unwinding_strong_erpm(f.value("unwinding_strong_ms").toDouble());
    f.endGroup();

    f.beginGroup("rewinding");
    meters_to_rewinding_trigger_length(f.value("rewinding_trigger_length").toDouble());
    kg_to_rewinding_current(f.value("rewinding_kg").toDouble());
    f.endGroup();

    f.beginGroup("slowing");
    meters_to_slowing_length(f.value("slowing_length").toDouble());
    kg_to_slowing_current(f.value("slowing_kg").toDouble());
    ms_to_slow_erpm(f.value("slow_ms").toDouble());
    kg_to_slow_max_current(f.value("slow_max_kg").toDouble());
    f.endGroup();

    f.beginGroup("winch");
    kg_to_pull_current(f.value("pull_kg").toDouble());
    percents_to_pre_pull_k(f.value("pre_pull_k").toInt());
    percents_to_takeoff_pull_k(f.value("takeoff_pull_k").toInt());
    percents_to_fast_pull_k(f.value("fast_pull_k").toInt());
    seconds_to_pre_pull_timeout(f.value("pre_pull_timeout").toDouble());
    meters_to_takeoff_trigger_length(f.value("takeoff_trigger_length").toDouble());
    seconds_to_takeoff_period(f.value("takeoff_period").toDouble());
    f.endGroup();

    f.beginGroup("manual");
    kg_to_manual_brake_current(f.value("manual_brake_kg").toDouble());
    ms_to_manual_slow_erpm(f.value("manual_slow_ms").toDouble());
    kg_to_manual_slow_speed_up_current(f.value("manual_slow_speed_up_kg").toDouble());
    kg_to_manual_slow_max_current(f.value("manual_slow_max_kg").toDouble());
    f.endGroup();

    f.beginGroup("antisex");
    kg_to_antisex_min_pull_amps(f.value("min_pull_kg").toDouble());
    kg_to_antisex_reduce_amps(f.value("reduce_kg").toDouble());
    antisex_acceleration_on_mss = f.value("acceleration_on_mss").toDouble();
    antisex_acceleration_off_mss = f.value("acceleration_off_mss").toDouble();
    seconds_to_antisex_max_period(f.value("max_period_secs").toDouble());
    f.endGroup();

    return f.status() == QSettings::NoError;
}
