#ifndef MISSING_TYPES_H
#define MISSING_TYPES_H

// vesc_tool and bldc datatypes.h files are different
// we need to add missing bldc types here

typedef enum {
        BATTERY_TYPE_LIION_3_0__4_2,
        BATTERY_TYPE_LIIRON_2_6__3_6,
        BATTERY_TYPE_LEAD_ACID
} BATTERY_TYPE;

#endif // MISSING_TYPES_H
