#ifndef VESC_LIVE_ACTIVITY_BRIDGE_H
#define VESC_LIVE_ACTIVITY_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// This feature requires iOS 18, matching Apple Watch Live Activity mirroring.
// All boolean results use 0/1. The returned error string remains valid until
// the next bridge operation changes the error; callers should copy it.
int vesc_live_activity_supported(void);
int vesc_live_activity_enabled(void);
int vesc_live_activity_start(const char *json);
void vesc_live_activity_update(const char *json);
void vesc_live_activity_end(void);
int vesc_live_activity_is_active(void);
const char *vesc_live_activity_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
