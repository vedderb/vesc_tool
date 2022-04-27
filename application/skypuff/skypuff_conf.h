#ifndef CONF_SKYPUFF_H_
#define CONF_SKYPUFF_H_

#include "datatypes.h"

typedef struct
{
    float motor_max_current;
    float charge_max_current;
    float discharge_max_current;
    float fet_temp_max;
    float motor_temp_max;
    float v_in_max;
    float v_in_min;
} skypuff_scales;

// Winch settings
typedef struct
{
    float amps_per_kg;					// Winch drive force coefficient
    int pull_applying_period;			// Milliseconds to apply pull force, amps_per_sec will be calculated from this delay
    int braking_applying_period;        // Milliseconds to release pull force when going Manual Braking
    int rope_length;					// Winch rope length in tachometer steps (used by interface only)
    int braking_length;					// Tachometer range of braking zone
    int braking_extension_length;		// Increase braking_length for passive winches when car drive 150m from takeoff
    int slowing_length;					// Range after braking zone to slow down motor when unwinding to zero
    float slow_erpm;					// Constant erpm in direction to zero
    int rewinding_trigger_length;		// Switch to fast rewinding state after going back this length
    int unwinding_trigger_length;		// Back to unwinding from rewinding if this range unwinded again
    float pull_current;					// Winch normal pull force, usually pilot weight
    float pre_pull_k;					// pre_pull_k * pull_current = pull current when pilots stays on the ground
    float takeoff_pull_k;				// takeoff_pull_k * pull_current = pull current during takeoff
    float fast_pull_k;					// fast_pull_k * pull_current = pull current to get altitude fast
    int takeoff_trigger_length;			// Minimal PRE_PULL movement for transition to TAKEOFF_PULL
    int pre_pull_timeout;				// Milliseconds timeout to save position after PRE_PULL
    int takeoff_period;					// Time of TAKEOFF_PULL and then switch to normal PULL
    float brake_current;				// Braking zone force, could be set high to charge battery driving away
    float slowing_current;				// Set zero to release motor when slowing or positive value to brake
    float manual_brake_current;			// Manual braking force
    float unwinding_current;			// Unwinding force
    float unwinding_strong_current;		// Due to motor cogging we need more powerfull unwinding near zero speed
    float unwinding_strong_erpm;			// Enable strong current unwinding if current speed is above
    float rewinding_current;			// Rewinding force
    float slow_max_current;				// Max force for constant slow speed
    float manual_slow_max_current;		// Max force for MANUAL_SLOW and MANUAL_SLOW_BACK
    float manual_slow_speed_up_current; // Speed up current for manual constant speed states
    float manual_slow_erpm;				// Constant speed for manual rotation
    float max_speed_ms;					// Speed scale limit (only affects the interface)

    // Antisex dampering
    float antisex_min_pull_amps;		// Activate antisex pull currection if pulling above this only
    float antisex_reduce_amps;			// Reduce motor amps to this value when antisex is activated
    float antisex_acceleration_on_mss;  // Activate antisex pull reduce if winding acceleration above this
    float antisex_acceleration_off_mss; // Deactivate antisex if current acceleration below this
    int antisex_max_period_ms;          // Do not reduce nominal pull more then this milliseconds
} skypuff_config;

// Drive settings part of mc_configuration
typedef struct
{
    int motor_poles;
    float wheel_diameter;
    float gear_ratio;

    // vesc_tool is not necessary to change battery limits
    BATTERY_TYPE battery_type;
    int battery_cells;

    /* This mc_configuration values will be updated according battery_type and cells number

    float l_battery_cut_start;
    float l_battery_cut_end;
    float l_max_vin;
    */
} skypuff_drive;

#endif