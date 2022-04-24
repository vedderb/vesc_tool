/*
    Copyright 2020 Kirill Kostiuchenko	kisel2626@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef QMLABLE_SKYPUFF_TYPES_H
#define QMLABLE_SKYPUFF_TYPES_H

#include <math.h>
#include <QObject>
#include <QSettings>

// Add missing bldc types before app_skypuff.h
#include "missing_types.h"
#include "app_skypuff.h"
#include "vbytearray.h"
#include "datatypes.h"

// Settings accesible to QML with on the fly units conversions
struct QMLable_skypuff_config : public skypuff_config, public skypuff_drive {
    Q_GADGET

    Q_PROPERTY(int motor_poles MEMBER motor_poles)
    Q_PROPERTY(int wheel_diameter_mm READ wheel_diameter_to_mm WRITE wheel_diameter_from_mm)
    Q_PROPERTY(float gear_ratio MEMBER gear_ratio)
    Q_PROPERTY(float amps_per_kg MEMBER amps_per_kg)
    Q_PROPERTY(float pull_applying_seconds READ pull_applying_period_to_seconds WRITE seconds_to_pull_applying_period)
    Q_PROPERTY(float braking_applying_seconds READ braking_applying_period_to_seconds WRITE seconds_to_braking_applying_period)
    Q_PROPERTY(float rope_length_meters READ rope_length_to_meters WRITE meters_to_rope_length)
    Q_PROPERTY(float braking_length_meters READ braking_length_to_meters WRITE meters_to_braking_length)
    Q_PROPERTY(float braking_extension_length_meters READ braking_extension_length_to_meters WRITE meters_to_braking_extension_length)
    Q_PROPERTY(float slowing_length_meters READ slowing_length_to_meters WRITE meters_to_slowing_length)
    Q_PROPERTY(float slow_erpm_ms READ slow_erpm_to_ms WRITE ms_to_slow_erpm)
    Q_PROPERTY(float rewinding_trigger_length_meters READ rewinding_trigger_length_to_meters WRITE meters_to_rewinding_trigger_length)
    Q_PROPERTY(float unwinding_trigger_length_meters READ unwinding_trigger_length_to_meters WRITE meters_to_unwinding_trigger_length)
    Q_PROPERTY(float motor_max_kg READ motor_max_current_to_kg)
    Q_PROPERTY(float power_max READ get_power_max)
    Q_PROPERTY(float power_min READ get_power_min)
    Q_PROPERTY(float pull_kg READ pull_current_to_kg WRITE kg_to_pull_current)
    Q_PROPERTY(int pre_pull_k_percents READ pre_pull_k_to_percents WRITE percents_to_pre_pull_k)
    Q_PROPERTY(int takeoff_pull_k_percents READ takeoff_pull_k_to_percents WRITE percents_to_takeoff_pull_k)
    Q_PROPERTY(int fast_pull_k_percents READ fast_pull_k_to_percents WRITE percents_to_fast_pull_k)
    Q_PROPERTY(float takeoff_trigger_length_meters READ takeoff_trigger_length_to_meters WRITE meters_to_takeoff_trigger_length)
    Q_PROPERTY(float pre_pull_timeout_seconds READ pre_pull_timeout_to_seconds WRITE seconds_to_pre_pull_timeout)
    Q_PROPERTY(float takeoff_period_seconds  READ takeoff_period_to_seconds WRITE seconds_to_takeoff_period)
    Q_PROPERTY(float brake_kg READ brake_current_to_kg WRITE kg_to_brake_current)
    Q_PROPERTY(float slowing_kg READ slowing_current_to_kg WRITE kg_to_slowing_current)
    Q_PROPERTY(float manual_brake_kg READ manual_brake_current_to_kg WRITE kg_to_manual_brake_current)
    Q_PROPERTY(float unwinding_kg READ unwinding_current_to_kg WRITE kg_to_unwinding_current)
    Q_PROPERTY(float unwinding_strong_kg READ unwinding_strong_current_to_kg WRITE kg_to_unwinding_strong_current)
    Q_PROPERTY(float unwinding_strong_ms READ unwinding_strong_erpm_to_ms WRITE ms_to_unwinding_strong_erpm)
    Q_PROPERTY(float rewinding_kg READ rewinding_current_to_kg WRITE kg_to_rewinding_current)
    Q_PROPERTY(float slow_max_kg READ slow_max_current_to_kg WRITE kg_to_slow_max_current)
    Q_PROPERTY(float manual_slow_max_kg READ manual_slow_max_current_to_kg WRITE kg_to_manual_slow_max_current)
    Q_PROPERTY(float manual_slow_speed_up_kg READ manual_slow_speed_up_current_to_kg WRITE kg_to_manual_slow_speed_up_current)
    Q_PROPERTY(float manual_slow_erpm_ms READ manual_slow_erpm_to_ms WRITE ms_to_manual_slow_erpm)
    Q_PROPERTY(float antisex_min_pull_kg READ antisex_min_pull_amps_to_kg WRITE kg_to_antisex_min_pull_amps)
    Q_PROPERTY(float antisex_reduce_kg READ antisex_reduce_amps_to_kg WRITE kg_to_antisex_reduce_amps)
    Q_PROPERTY(float antisex_acceleration_on_mss MEMBER antisex_acceleration_on_mss)
    Q_PROPERTY(float antisex_acceleration_off_mss MEMBER antisex_acceleration_off_mss)
    Q_PROPERTY(float antisex_max_period_seconds READ antisex_max_period_to_seconds WRITE seconds_to_antisex_max_period)
    Q_PROPERTY(int battery_type MEMBER battery_type)
    Q_PROPERTY(int battery_cells MEMBER battery_cells)
    Q_PROPERTY(float max_speed_ms MEMBER max_speed_ms)
public:

    float motor_max_current;
    float charge_max_current, discharge_max_current;
    float v_in_max, v_in_min;
    float fet_temp_max, motor_temp_max, bat_temp_max;
    int battery_cells;
    int battery_type; // Temporary, until BATTERY_TYPE will be moved to datatypes.h of vesc_tool

    QMLable_skypuff_config()
    {
        clearScales();
    }

    void clearScales()
    {
        amps_per_kg = 0;
        rope_length = 0;
        motor_max_current = 0;
        charge_max_current = 0;
        discharge_max_current = 0;
        v_in_max = 0;
        v_in_min = 0;
        fet_temp_max = 0;
        motor_temp_max = 0;
        bat_temp_max = 0;
    }

    float get_power_max() const {return v_in_max * discharge_max_current;}
    // Temporary
    float get_power_min() const {return v_in_max * charge_max_current;}

    int wheel_diameter_to_mm() const {return round(wheel_diameter * (float)1000);}
    void wheel_diameter_from_mm(int mm) {wheel_diameter = (float)mm / (float)1000;}

    int pre_pull_k_to_percents() const {return round(pre_pull_k * (float)100);}
    void percents_to_pre_pull_k(int p) {pre_pull_k = (float)p / (float)100;}

    int takeoff_pull_k_to_percents() const {return round(takeoff_pull_k * (float)100);}
    void percents_to_takeoff_pull_k(int p) {takeoff_pull_k = (float)p / (float)100;}

    int fast_pull_k_to_percents() const {return round(fast_pull_k * (float)100);}
    void percents_to_fast_pull_k(int p) {fast_pull_k = (float)p / (float)100;}

    float pre_pull_timeout_to_seconds() const {return pre_pull_timeout / (float)1000;}
    void seconds_to_pre_pull_timeout(float secs) {pre_pull_timeout = round(secs * (float)1000);}

    float takeoff_period_to_seconds() const {return takeoff_period / (float)1000;}
    void seconds_to_takeoff_period(float secs) {takeoff_period = round(secs * (float)1000);}

    float pull_applying_period_to_seconds() const {return pull_applying_period / (float)1000;}
    void seconds_to_pull_applying_period(float secs) {pull_applying_period = round(secs * (float)1000);}

    float braking_applying_period_to_seconds() const {return braking_applying_period / (float)1000;}
    void seconds_to_braking_applying_period(float secs) {braking_applying_period = round(secs * (float)1000);}

    float motor_max_current_to_kg() const {return motor_max_current / amps_per_kg;}

    float pull_current_to_kg() const {return pull_current / amps_per_kg;}
    void kg_to_pull_current(float kg) {pull_current = kg * amps_per_kg;}

    float brake_current_to_kg() const {return brake_current / amps_per_kg;}
    void kg_to_brake_current(float kg) {brake_current = kg * amps_per_kg;}

    float manual_brake_current_to_kg() const {return manual_brake_current / amps_per_kg;}
    void kg_to_manual_brake_current(float kg) {manual_brake_current = kg * amps_per_kg;}

    float unwinding_current_to_kg() const {return unwinding_current / amps_per_kg;}
    void kg_to_unwinding_current(float kg) {unwinding_current = kg * amps_per_kg;}

    float unwinding_strong_current_to_kg() const {return unwinding_strong_current / amps_per_kg;}
    void kg_to_unwinding_strong_current(float kg) {unwinding_strong_current = kg * amps_per_kg;}

    float unwinding_strong_erpm_to_ms() const {return erpm_to_ms(unwinding_strong_erpm);}
    void ms_to_unwinding_strong_erpm(float ms) {unwinding_strong_erpm = ms_to_erpm(ms);}

    float rewinding_current_to_kg() const {return rewinding_current / amps_per_kg;}
    void kg_to_rewinding_current(float kg) {rewinding_current = kg * amps_per_kg;}

    float slowing_current_to_kg() const {return slowing_current / amps_per_kg;}
    void kg_to_slowing_current(float kg) {slowing_current = kg * amps_per_kg;}

    float slow_max_current_to_kg() const {return slow_max_current / amps_per_kg;}
    void kg_to_slow_max_current(float kg) {slow_max_current = kg * amps_per_kg;}

    float manual_slow_max_current_to_kg() const {return manual_slow_max_current / amps_per_kg;}
    void kg_to_manual_slow_max_current(float kg) {manual_slow_max_current = kg * amps_per_kg;}

    float manual_slow_speed_up_current_to_kg() const {return manual_slow_speed_up_current / amps_per_kg;}
    void kg_to_manual_slow_speed_up_current(float kg) {manual_slow_speed_up_current = kg * amps_per_kg;}

    float antisex_min_pull_amps_to_kg() const {return antisex_min_pull_amps / amps_per_kg;}
    void kg_to_antisex_min_pull_amps(float kg) {antisex_min_pull_amps = kg * amps_per_kg;}

    float antisex_reduce_amps_to_kg() const {return antisex_reduce_amps / amps_per_kg;}
    void kg_to_antisex_reduce_amps(float kg) {antisex_reduce_amps = kg * amps_per_kg;}

    float antisex_max_period_to_seconds() const {return antisex_max_period_ms / (float)1000;}
    void seconds_to_antisex_max_period(float secs) {antisex_max_period_ms = round(secs * (float)1000);}

    float rope_length_to_meters() const {return tac_steps_to_meters(rope_length);}
    void meters_to_rope_length(float meters) {rope_length = meters_to_tac_steps(meters);}

    float braking_length_to_meters() const {return tac_steps_to_meters(braking_length);}
    void meters_to_braking_length(float meters) {braking_length = meters_to_tac_steps(meters);}

    float braking_extension_length_to_meters() const {return tac_steps_to_meters(braking_extension_length);}
    void meters_to_braking_extension_length(float meters) {braking_extension_length = meters_to_tac_steps(meters);}

    float slowing_length_to_meters() const {return tac_steps_to_meters(slowing_length);}
    void meters_to_slowing_length(float meters) {slowing_length = meters_to_tac_steps(meters);}

    float rewinding_trigger_length_to_meters() const {return tac_steps_to_meters(rewinding_trigger_length);}
    void meters_to_rewinding_trigger_length(float meters) {rewinding_trigger_length = meters_to_tac_steps(meters);}

    float unwinding_trigger_length_to_meters() const {return tac_steps_to_meters(unwinding_trigger_length);}
    void meters_to_unwinding_trigger_length(float meters) {unwinding_trigger_length = meters_to_tac_steps(meters);}

    float takeoff_trigger_length_to_meters() const {return tac_steps_to_meters(takeoff_trigger_length);}
    void meters_to_takeoff_trigger_length(float meters) {takeoff_trigger_length = meters_to_tac_steps(meters);}

    float slow_erpm_to_ms() const {return erpm_to_ms(slow_erpm);}
    void ms_to_slow_erpm(float ms) {slow_erpm = ms_to_erpm(ms);}

    float manual_slow_erpm_to_ms() const {return erpm_to_ms(manual_slow_erpm);}
    void ms_to_manual_slow_erpm(float ms) {manual_slow_erpm = ms_to_erpm(ms);}

    inline float meters_per_rev() const {return wheel_diameter / gear_ratio * M_PI;}
    inline float steps_per_rev(void) const {return motor_poles * 3;}
    inline int meters_to_tac_steps(float meters) const {return round(meters / meters_per_rev() * steps_per_rev());}
    inline float tac_steps_to_meters(int steps) const {return (float)steps / steps_per_rev() * meters_per_rev();}
    inline float ms_to_erpm(float ms) const
    {
        float rps = ms / meters_per_rev();
        float rpm = rps * 60;

        return rpm * (motor_poles / 2);
    }
    inline float erpm_to_ms(float erpm) const
    {
        float erps = erpm / 60;
        float rps = erps / (motor_poles / 2);

        return rps * meters_per_rev();
    }

    QByteArray serializeV1() const;
    bool deserializeV1(VByteArray& from);

    bool saveV1(QSettings &f) const;
    bool loadV1(QSettings &f);
};

Q_DECLARE_METATYPE(QMLable_skypuff_config)


#endif
