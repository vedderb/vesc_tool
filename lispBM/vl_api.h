/*
    Copyright 2026 Jeffrey M. Friesen

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#ifndef VL_API_H
#define VL_API_H

#include <stdint.h>
#include <stdbool.h>
#include "lispBM/repl/vesc_mock.h"

#ifdef __cplusplus
extern "C" {
#endif

// The simulated hardware, called from the evaluator thread
typedef struct {
    void (*print)(void *host, const char *str);
    // send-data, can-send-sid, can-send-eid and canmsg-send. arg is the slot of canmsg-send.
    void (*send)(void *host, const char *what, int id, int arg, const uint8_t *data, int len);
    // canget-*, can-msg-age, can-local-id, bms-*, ads-volts and systime-hz, NAN when not available
    double (*get)(void *host, const char *what, int id);
    // set-*, canset-*, timeout-reset and reboot. id is -1 for the own motor.
    void (*set)(void *host, const char *what, int id, double value);
    // Returns true when the value is an integer
    bool (*conf)(void *host, const char *name, double *value, bool set, bool def);
    bool (*eeprom)(void *host, int addr, uint32_t *value, bool store);
    // An image in RGB888, or the display cleared to color when rgb is NULL
    void (*display)(void *host, int x, int y, int w, int h, const uint32_t *rgb, uint32_t color);
} vl_host_t;

// One LispBM runtime. Everything but run is called from the Qt thread.
typedef struct {
    bool (*init)(const vl_host_t *fns, void *host);
    void (*run)(void); // The evaluator thread, returns after kill
    void (*kill)(void);
    // The code slot payload: code, imports and their data. Must stay valid while running.
    bool (*load)(const char *code, int len);
    bool (*eval)(const char *str, bool program);
    void (*stats)(float *heap, float *mem, void *host, void (*global)(void *host, const char *name, double value));
    void (*mock)(const vesc_mock_state_t *state);
    // data-rx, can-sid, can-eid and rmsg
    void (*event)(const char *what, int id, int arg, const uint8_t *data, int len);
} vl_api_t;

extern const vl_api_t vl0_api, vl1_api, vl2_api, vl3_api, vl4_api;

#ifdef __cplusplus
}
#endif

#endif // VL_API_H
