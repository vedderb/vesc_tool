/*
    Copyright 2026 Benjamin Vedder	benjamin@vedder.se

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

/*
 * One LispBM runtime of the Virtual ESC, with the firmware extensions that
 * matter for testing a vehicle. The rest are the nil-stubs and mock values
 * from the LispBM REPL.
 *
 * LispBM keeps its state in globals, so this file is compiled once per
 * simulated device (vl_node0.c, ...) with all global symbols prefixed by
 * lbm_rename.h. The only symbol left is VL_PFX_api.
 */

#include <stdint.h>
#if UINTPTR_MAX > 0xFFFFFFFFu
#define LBM64
#endif

// Same as the firmware
#define LBM_USE_DYN_FUNS
#define LBM_USE_DYN_MACROS
#define LBM_USE_DYN_LOOPS
#define LBM_USE_TIME_QUOTA
#define LBM_USE_ERROR_LINENO
#define LBM_USE_MACRO_REST_ARGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef _WIN32
#include <locale.h>
#ifdef __APPLE__
#include <xlocale.h>
#endif
#endif

#define VL_CAT2(a, b) a##_##b
#define VL_CAT(a, b) VL_CAT2(a, b)
#define VL_NAME(name) VL_CAT(VL_PFX, name)
#ifndef VL_NO_RENAME
#include "lbm_rename.h"
#endif


// isnanf is missing on some platforms
#define isnanf(x) isnan(x)
#define isinff(x) isinf(x)

// Separate files upstream, so a few names clash in one file
#include "lispBM/utils/tinygfx.c"
#define indexed4_mask sft_indexed4_mask
#define indexed4_shift sft_indexed4_shift
#define indexed16_mask sft_indexed16_mask
#define indexed16_shift sft_indexed16_shift
#include "lispBM/src/extensions/schrift.c"
#undef indexed4_mask
#undef indexed4_shift
#undef indexed16_mask
#undef indexed16_shift

#include "lispBM/src/env.c"
#include "lispBM/src/eval_cps.c"
#include "lispBM/src/extensions.c"
#include "lispBM/src/fundamental.c"
#include "lispBM/src/heap.c"
#include "lispBM/src/lbm_c_interop.c"
#include "lispBM/src/lbm_channel.c"
#include "lispBM/src/lbm_custom_type.c"
#include "lispBM/src/lbm_defrag_mem.c"
#include "lispBM/src/lbm_flat_value.c"
#include "lispBM/src/lbm_image.c"
#include "lispBM/src/lbm_memory.c"
#include "lispBM/src/lbm_prof.c"
#include "lispBM/src/lispbm.c"
#include "lispBM/src/print.c"
#include "lispBM/src/stack.c"
#include "lispBM/src/symrepr.c"
#include "lispBM/src/tokpar.c"
#include "lispBM/src/extensions/array_extensions.c"
#include "lispBM/src/extensions/display_extensions.c"
#include "lispBM/src/extensions/lbm_dyn_lib.c"
#include "lispBM/src/extensions/math_extensions.c"
#include "lispBM/src/extensions/mutex_extensions.c"
#include "lispBM/src/extensions/random_extensions.c"
#include "lispBM/src/extensions/runtime_extensions.c"
#include "lispBM/src/extensions/set_extensions.c"
#include "lispBM/src/extensions/string_extensions.c"
#include "lispBM/src/extensions/ttf_backend.c"
#include "lispBM/src/extensions/ttf_extensions.c"
#include "lispBM/utils/buffer.c"
#include "lispBM/utils/tjpgd.c"
#include "lispBM/repl/vesc_extension_stubs.c"
#include "express/lbm_color_extensions.c"
#include "vl_api.h"

#define HEAP_CELLS 16384
#define MEM_BLOCKS 8192
#define IMAGE_WORDS 65536
#define EXT_MAX 700
#define RMSG_SLOTS 5

// Shared by all runtimes
extern void vl_sleep_us(uint32_t us);
extern uint64_t vl_time_us(void);

static const vl_host_t *fns;
static void *host;

static lbm_cons_t vl_heap[HEAP_CELLS];
static lbm_uint vl_memory[LBM_MEMORY_SIZE_BLOCKS_TO_WORDS(MEM_BLOCKS)];
static lbm_uint vl_bitmap[LBM_MEMORY_BITMAP_SIZE(MEM_BLOCKS)];
static uint32_t vl_image[IMAGE_WORDS];
static lbm_extension_t vl_extensions[EXT_MAX];

// The code, the REPL and streamed code each have their own reader, like in the firmware
static lbm_string_channel_state_t tok_state, repl_state, stream_state;
static lbm_char_channel_t tok, repl_tok, stream_tok;
static char *repl_buf, *stream_buf;
static volatile lbm_cid repl_cid = -1;

// canmsg-recv, the start and timeout tell late replies apart
static lbm_mutex_t rmsg_mutex;
static lbm_cid rmsg_cid[RMSG_SLOTS];
static uint32_t rmsg_start[RMSG_SLOTS], rmsg_timeout[RMSG_SLOTS];

// Enabled events, 0 when off. Looked up here as the symbol list is not thread safe.
static volatile lbm_uint sym_can_sid, sym_can_eid, sym_data_rx;

static char print_prefix[50];
static int ads_channel;
static uint32_t systime_hz;
static uint64_t boot_us;

static int print_cb(const char *fmt, ...) {
    char buf[400];
    int n = snprintf(buf, sizeof(buf), "%s", print_prefix);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
    va_end(ap);
    fns->print(host, buf);
    return 1;
}

static void done_cb(eval_context_t *ctx) {
    if (ctx->id == repl_cid) {
        char out[128];
        lbm_print_value(out, sizeof(out), ctx->r);
        print_cb("> %s", out);
        repl_cid = -1;
    }
}

static bool vl_image_write(uint32_t w, int32_t ix, bool const_heap) {
    (void)const_heap;
    if (vl_image[ix] != 0xFFFFFFFF && vl_image[ix] != w) {
        return false;
    }
    vl_image[ix] = w;
    return true;
}

static bool pause_eval(void) {
    lbm_pause_eval();
    uint32_t start = lbm_timestamp();
    while (lbm_get_eval_state() != EVAL_CPS_STATE_PAUSED && lbm_timestamp() - start < 2000000) {
        vl_sleep_us(10);
    }
    return lbm_get_eval_state() == EVAL_CPS_STATE_PAUSED;
}

// The evaluator sleeps 200 us when idle, which wakes the phone too often
static void eval_sleep(uint32_t us) {
    vl_sleep_us(us < 1000 ? 1000 : us);
}

// Byte arrays and lists of numbers
static int to_bytes(lbm_value v, uint8_t *buf, int max) {
    if (lbm_is_array_r(v)) {
        lbm_array_header_t *a = (lbm_array_header_t*)lbm_car(v);
        int len = a->size < (lbm_uint)max ? (int)a->size : max;
        memcpy(buf, a->data, len);
        return len;
    }

    int len = 0;
    for (lbm_value c = v;lbm_is_cons(c) && len < max;c = lbm_cdr(c)) {
        buf[len++] = (uint8_t)lbm_dec_as_i32(lbm_car(c));
    }
    return len;
}

static lbm_value vx_print(lbm_value *args, lbm_uint argn) {
    char buf[256];
    for (lbm_uint i = 0;i < argn;i++) {
        lbm_print_value(buf, sizeof(buf), args[i]);
        print_cb("%s", buf);
    }
    return ENC_SYM_TRUE;
}

static lbm_value vx_puts(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN(1);
    if (!lbm_is_array_r(args[0])) {
        return ENC_SYM_TERROR;
    }
    print_cb("%s", lbm_dec_str(args[0]));
    return ENC_SYM_TRUE;
}

static lbm_value vx_set_print_prefix(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN(1);
    if (!lbm_is_array_r(args[0])) {
        return ENC_SYM_TERROR;
    }
    strncpy(print_prefix, lbm_dec_str(args[0]), sizeof(print_prefix) - 1);
    return ENC_SYM_TRUE;
}

// Ticks since boot, 10 kHz on the motor controllers and 1 kHz on the Express
static uint32_t systime(void) {
    return (uint32_t)((vl_time_us() - boot_us) * systime_hz / 1000000);
}

static lbm_value vx_systime(lbm_value *args, lbm_uint argn) {
    (void)args; (void)argn;
    return lbm_enc_u32(systime());
}

static lbm_value vx_secs_since(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN_NUMBER(1);
    return lbm_enc_float((float)(uint32_t)(systime() - lbm_dec_as_u32(args[0])) / (float)systime_hz);
}

static lbm_value enc_bits(uint32_t v) {
    return v > ((1 << 27) - 1) ? lbm_enc_i32((int32_t)v) : lbm_enc_i(v);
}

static lbm_value vx_bits_enc_int(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN_NUMBER(4);
    uint32_t initial = lbm_dec_as_u32(args[0]);
    uint32_t offset = lbm_dec_as_u32(args[1]);
    uint32_t number = lbm_dec_as_u32(args[2]);
    uint32_t bits = lbm_dec_as_u32(args[3]);
    initial &= ~((0xFFFFFFFF >> (32 - bits)) << offset);
    initial |= (number << (32 - bits)) >> (32 - bits - offset);
    return enc_bits(initial);
}

static lbm_value vx_bits_dec_int(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN_NUMBER(3);
    uint32_t val = lbm_dec_as_u32(args[0]) >> lbm_dec_as_u32(args[1]);
    return enc_bits(val & (0xFFFFFFFF >> (32 - lbm_dec_as_u32(args[2]))));
}

static lbm_value vx_event_enable(lbm_value *args, lbm_uint argn) {
    if ((argn != 1 && argn != 2) || !lbm_is_symbol(args[0])) {
        return ENC_SYM_EERROR;
    }
    lbm_uint sym = (argn == 1 || lbm_dec_as_i32(args[1])) ? lbm_dec_sym(args[0]) : 0;
    const char *name = lbm_get_name_by_symbol(lbm_dec_sym(args[0]));
    if (strcmp(name, "event-can-sid") == 0) {
        sym_can_sid = sym;
    } else if (strcmp(name, "event-can-eid") == 0) {
        sym_can_eid = sym;
    } else if (strcmp(name, "event-data-rx") == 0) {
        sym_data_rx = sym;
    }
    return ENC_SYM_TRUE;
}

// (send-data data), (can-send-sid/eid id data) and (canmsg-send id slot data)
static lbm_value vl_send(const char *what, int id, int arg, lbm_value data, int max) {
    uint8_t buf[500];
    fns->send(host, what, id, arg, buf, to_bytes(data, buf, max));
    return ENC_SYM_TRUE;
}

static lbm_value vx_send_data(lbm_value *args, lbm_uint argn) {
    return argn >= 1 ? vl_send("send-data", 0, 0, args[0], 500) : ENC_SYM_TERROR;
}

static lbm_value vx_can_send_sid(lbm_value *args, lbm_uint argn) {
    return argn == 2 ? vl_send("can-send-sid", lbm_dec_as_i32(args[0]), 0, args[1], 8) : ENC_SYM_TERROR;
}

static lbm_value vx_can_send_eid(lbm_value *args, lbm_uint argn) {
    return argn == 2 ? vl_send("can-send-eid", lbm_dec_as_i32(args[0]), 0, args[1], 8) : ENC_SYM_TERROR;
}

static lbm_value vx_canmsg_send(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN(3);
    return vl_send("canmsg-send", lbm_dec_as_i32(args[0]), lbm_dec_as_i32(args[1]), args[2], 500);
}

// (canmsg-recv slot timeout), blocks until canmsg-send to the slot or the timeout
static lbm_value vx_canmsg_recv(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN_NUMBER(2);
    int slot = lbm_dec_as_i32(args[0]);
    float timeout = lbm_dec_as_float(args[1]);
    if (slot < 0 || slot >= RMSG_SLOTS) {
        return ENC_SYM_TERROR;
    }
    lbm_mutex_lock(&rmsg_mutex);
    rmsg_cid[slot] = lbm_get_current_cid();
    rmsg_start[slot] = lbm_timestamp();
    rmsg_timeout[slot] = timeout > 0.0f ? (uint32_t)(timeout * 1e6f) : 0;
    lbm_mutex_unlock(&rmsg_mutex);
    if (timeout > 0.0f) {
        lbm_block_ctx_from_extension_timeout(timeout);
    } else {
        lbm_block_ctx_from_extension();
    }
    return ENC_SYM_TRUE;
}

static lbm_value vx_can_local_id(lbm_value *args, lbm_uint argn) {
    (void)args; (void)argn;
    return lbm_enc_i((lbm_int)fns->get(host, "can-local-id", 0));
}

static lbm_value vx_can_list_devs(lbm_value *args, lbm_uint argn) {
    (void)args; (void)argn;
    lbm_value res = ENC_SYM_NIL;
    for (int i = 254;i >= 0;i--) {
        if (!isnan(fns->get(host, "can-msg-age", i))) {
            res = lbm_cons(lbm_enc_i(i), res);
        }
    }
    return res;
}

// (canget-x id), (can-msg-age id msg) and (get-bms-val sym [ind]). Like the
// firmware, values of devices that are not on the bus are 0.
static lbm_value vl_get(const char *what, int id) {
    double v = fns->get(host, what, id);
    return lbm_enc_float(isnan(v) ? 0.0f : (float)v);
}

#define CANGET_LIST \
    X(rpm, "rpm") X(current, "current") X(current_dir, "current-dir") X(current_in, "current-in") \
    X(duty, "duty") X(temp_fet, "temp-fet") X(temp_motor, "temp-motor") X(vin, "vin") \
    X(speed, "speed") X(dist, "dist") X(ppm, "ppm") X(adc, "adc")
#define X(n, name) static lbm_value vx_canget_##n(lbm_value *args, lbm_uint argn) { \
    return argn >= 1 && lbm_is_number(args[0]) ? vl_get(name, lbm_dec_as_i32(args[0])) : ENC_SYM_TERROR; }
CANGET_LIST
#undef X

static lbm_value vx_can_msg_age(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN_NUMBER(2);
    double age = fns->get(host, "can-msg-age", lbm_dec_as_i32(args[0]));
    return isnan(age) ? ENC_SYM_NIL : lbm_enc_float((float)age);
}

static lbm_value vx_get_bms_val(lbm_value *args, lbm_uint argn) {
    if (argn < 1 || !lbm_is_symbol(args[0])) {
        return ENC_SYM_TERROR;
    }
    return vl_get(lbm_get_name_by_symbol(lbm_dec_sym(args[0])), argn > 1 ? lbm_dec_as_i32(args[1]) : 0);
}

// (set-current val) and (canset-current id val), the rest of the motor commands
// alike. Further arguments, e.g. the off delay of set-current, are ignored.
static lbm_value vl_set(const char *what, int id, lbm_value val) {
    if (!lbm_is_number(val)) {
        return ENC_SYM_TERROR;
    }
    fns->set(host, what, id, lbm_dec_as_double(val));
    return ENC_SYM_TRUE;
}

#define SET_LIST \
    X(current, "current") X(current_rel, "current-rel") X(brake, "brake") X(brake_rel, "brake-rel") \
    X(duty, "duty") X(rpm, "rpm")
#define X(n, name) \
    static lbm_value vx_set_##n(lbm_value *args, lbm_uint argn) { \
    return argn >= 1 ? vl_set(name, -1, args[0]) : ENC_SYM_TERROR; } \
    static lbm_value vx_canset_##n(lbm_value *args, lbm_uint argn) { \
    return argn >= 2 && lbm_is_number(args[0]) ? vl_set(name, lbm_dec_as_i32(args[0]), args[1]) : ENC_SYM_TERROR; }
SET_LIST
#undef X

// (sysinfo 'odometer) and (sysinfo 'fw-ver), the others are nil
static lbm_value vx_sysinfo(lbm_value *args, lbm_uint argn) {
    if (argn != 1 || !lbm_is_symbol(args[0])) {
        return ENC_SYM_TERROR;
    }
    const char *name = lbm_get_name_by_symbol(lbm_dec_sym(args[0]));
    if (strcmp(name, "odometer") == 0) {
        return lbm_enc_u32((uint32_t)fns->get(host, "odometer", 0));
    } else if (strcmp(name, "fw-ver") == 0) {
        return lbm_cons(lbm_enc_i((lbm_int)fns->get(host, "fw-major", 0)),
                        lbm_cons(lbm_enc_i((lbm_int)fns->get(host, "fw-minor", 0)), lbm_cons(lbm_enc_i(0), ENC_SYM_NIL)));
    }
    return ENC_SYM_NIL;
}

// The fault code, the stub returns its name
static lbm_value vx_get_fault(lbm_value *args, lbm_uint argn) {
    (void)args; (void)argn;
    return lbm_enc_i((lbm_int)fns->get(host, "fault", 0));
}

static lbm_value vx_timeout_reset(lbm_value *args, lbm_uint argn) {
    (void)args; (void)argn;
    fns->set(host, "alive", -1, 0.0);
    return ENC_SYM_TRUE;
}

static lbm_value vx_reboot(lbm_value *args, lbm_uint argn) {
    (void)args; (void)argn;
    fns->set(host, "reboot", -1, 0.0);
    return ENC_SYM_TRUE;
}

// conf-get and conf-set by the name in the configuration XML, with - for _
static lbm_value vl_conf(lbm_value *args, lbm_uint argn, bool set) {
    if (argn < 1 || !lbm_is_symbol(args[0]) || (set && (argn != 2 || !lbm_is_number(args[1])))) {
        return ENC_SYM_TERROR;
    }

    char name[100];
    strncpy(name, lbm_get_name_by_symbol(lbm_dec_sym(args[0])), sizeof(name) - 1);
    name[sizeof(name) - 1] = 0;
    for (char *c = name;*c;c++) {
        *c = *c == '-' ? '_' : *c;
    }

    double val = set ? lbm_dec_as_double(args[1]) : 0.0;
    bool def = !set && argn == 2 && lbm_dec_as_i32(args[1]);
    bool isInt = fns->conf(host, name, &val, set, def);
    return set ? ENC_SYM_TRUE : (isInt ? lbm_enc_i((lbm_int)val) : lbm_enc_float((float)val));
}

static lbm_value vx_conf_get(lbm_value *args, lbm_uint argn) {
    return vl_conf(args, argn, false);
}

static lbm_value vx_conf_set(lbm_value *args, lbm_uint argn) {
    return vl_conf(args, argn, true);
}

static lbm_value vl_eeprom(lbm_value *args, lbm_uint argn, bool store, bool isFloat) {
    if (argn != (store ? 2u : 1u) || !lbm_is_number(args[0]) || (store && !lbm_is_number(args[1]))) {
        return ENC_SYM_TERROR;
    }

    union {uint32_t u; int32_t i; float f;} v;
    if (store) {
        if (isFloat) {
            v.f = lbm_dec_as_float(args[1]);
        } else {
            v.i = lbm_dec_as_i32(args[1]);
        }
    }

    if (!fns->eeprom(host, lbm_dec_as_i32(args[0]), &v.u, store)) {
        return ENC_SYM_NIL;
    }
    return store ? ENC_SYM_TRUE : (isFloat ? lbm_enc_float(v.f) : lbm_enc_i32(v.i));
}

static lbm_value vx_eeprom_store_f(lbm_value *args, lbm_uint argn) { return vl_eeprom(args, argn, true, true); }
static lbm_value vx_eeprom_read_f(lbm_value *args, lbm_uint argn) { return vl_eeprom(args, argn, false, true); }
static lbm_value vx_eeprom_store_i(lbm_value *args, lbm_uint argn) { return vl_eeprom(args, argn, true, false); }
static lbm_value vx_eeprom_read_i(lbm_value *args, lbm_uint argn) { return vl_eeprom(args, argn, false, false); }

// The ADS1115 that the Dash reads its buttons with. Config writes select the
// channel, reads return its voltage.
static lbm_value vx_i2c_tx_rx(lbm_value *args, lbm_uint argn) {
    if (argn < 2 || lbm_dec_as_i32(args[0]) != 0x48) {
        return ENC_SYM_NIL;
    }

    uint8_t tx[8];
    int txLen = to_bytes(args[1], tx, sizeof(tx));
    if (txLen == 3 && tx[0] == 1) {
        ads_channel = (tx[1] >> 4) & 0x03;
    }

    if (argn > 2 && lbm_is_array_rw(args[2])) {
        lbm_array_header_t *rx = (lbm_array_header_t*)lbm_car(args[2]);
        int16_t raw = (int16_t)(fns->get(host, "ads-volts", ads_channel) / 4.096 * 32767.0);
        if (rx->size >= 2) {
            ((uint8_t*)rx->data)[0] = (uint8_t)(raw >> 8);
            ((uint8_t*)rx->data)[1] = (uint8_t)raw;
        }
    }
    return ENC_SYM_TRUE;
}

static lbm_value vx_load_native_lib(lbm_value *args, lbm_uint argn) {
    (void)args; (void)argn;
    print_cb("load-native-lib: native code does not run in the Virtual ESC");
    return ENC_SYM_NIL;
}

static lbm_value vx_true(lbm_value *args, lbm_uint argn) {
    (void)args; (void)argn;
    return ENC_SYM_TRUE;
}

static bool render_image(image_buffer_t *img, uint16_t x, uint16_t y, color_t *colors) {
    uint32_t *rgb = malloc(sizeof(uint32_t) * img->width * img->height);
    if (!rgb) {
        return false;
    }

    for (int j = 0;j < img->height;j++) {
        for (int i = 0;i < img->width;i++) {
            // RGB formats are RGB888 already, indexed ones use the colors
            uint32_t c = getpixel(img, i, j);
            if (img->fmt == indexed2 || img->fmt == indexed4 || img->fmt == indexed16) {
                c = COLOR_TO_RGB888(colors[c], i, j);
            }
            rgb[j * img->width + i] = c;
        }
    }

    fns->display(host, x, y, img->width, img->height, rgb, 0);
    free(rgb);
    return true;
}

static void clear_display(uint32_t color) {
    fns->display(host, 0, 0, 0, 0, NULL, color);
}

static void reset_display(void) {
    clear_display(0);
}

static bool dyn_load(const char *str, const char **code) {
    return lbm_dyn_lib_find(str, code);
}

static bool vl_init(const vl_host_t *f, void *h) {
    fns = f;
    host = h;
    free(repl_buf);
    free(stream_buf);
    repl_buf = stream_buf = NULL;
    repl_cid = -1;
    sym_can_sid = sym_can_eid = sym_data_rx = 0;
    print_prefix[0] = 0;
    boot_us = vl_time_us();
    systime_hz = (uint32_t)fns->get(host, "systime-hz", 0);
    if (!rmsg_mutex.handle) {
        lbm_mutex_init(&rmsg_mutex);
    }
    for (int i = 0;i < RMSG_SLOTS;i++) {
        rmsg_cid[i] = -1;
    }

    if (!lbm_init(vl_heap, HEAP_CELLS, vl_memory, LBM_MEMORY_SIZE_BLOCKS_TO_WORDS(MEM_BLOCKS),
                  vl_bitmap, LBM_MEMORY_BITMAP_SIZE(MEM_BLOCKS), 256, 256, vl_extensions, EXT_MAX)) {
        return false;
    }

    lbm_set_usleep_callback(eval_sleep);
    lbm_set_printf_callback(print_cb);
    lbm_set_ctx_done_callback(done_cb);
    lbm_set_dynamic_load_callback(dyn_load);

    memset(vl_image, 0xFF, sizeof(vl_image));
    lbm_image_init(vl_image, IMAGE_WORDS, vl_image_write);
    lbm_image_create("virtual");
    lbm_image_boot();
    lbm_add_eval_symbols();
    lbm_eval_init_events(30);

    lbm_array_extensions_init();
    lbm_math_extensions_init();
    lbm_string_extensions_init();
    lbm_mutex_extensions_init();
    lbm_runtime_extensions_init();
    lbm_random_extensions_init();
    lbm_set_extensions_init();
    lbm_display_extensions_init();
    lbm_ttf_extensions_init();
    lbm_dyn_lib_init();
    lbm_display_extensions_set_callbacks(render_image, clear_display, reset_display);
    lbm_color_extensions_init();

    load_vesc_extension_stubs();
    lbm_add_extension("print", vx_print);
    lbm_add_extension("puts", vx_puts);
    lbm_add_extension("set-print-prefix", vx_set_print_prefix);
    lbm_add_extension("systime", vx_systime);
    lbm_add_extension("secs-since", vx_secs_since);
    lbm_add_extension("bits-enc-int", vx_bits_enc_int);
    lbm_add_extension("bits-dec-int", vx_bits_dec_int);
    lbm_add_extension("event-enable", vx_event_enable);
    lbm_add_extension("send-data", vx_send_data);
    lbm_add_extension("can-send-sid", vx_can_send_sid);
    lbm_add_extension("can-send-eid", vx_can_send_eid);
    lbm_add_extension("canmsg-send", vx_canmsg_send);
    lbm_add_extension("canmsg-recv", vx_canmsg_recv);
    lbm_add_extension("can-local-id", vx_can_local_id);
    lbm_add_extension("can-list-devs", vx_can_list_devs);
    lbm_add_extension("can-msg-age", vx_can_msg_age);
    lbm_add_extension("get-bms-val", vx_get_bms_val);
    lbm_add_extension("get-fault", vx_get_fault);
    lbm_add_extension("sysinfo", vx_sysinfo);
    lbm_add_extension("timeout-reset", vx_timeout_reset);
    lbm_add_extension("reboot", vx_reboot);
    lbm_add_extension("conf-get", vx_conf_get);
    lbm_add_extension("conf-set", vx_conf_set);
    lbm_add_extension("eeprom-store-f", vx_eeprom_store_f);
    lbm_add_extension("eeprom-read-f", vx_eeprom_read_f);
    lbm_add_extension("eeprom-store-i", vx_eeprom_store_i);
    lbm_add_extension("eeprom-read-i", vx_eeprom_read_i);
    lbm_add_extension("i2c-tx-rx", vx_i2c_tx_rx);
    lbm_add_extension("load-native-lib", vx_load_native_lib);
#define X(n, name) lbm_add_extension("canget-" name, vx_canget_##n);
    CANGET_LIST
#undef X
#define X(n, name) lbm_add_extension("set-" name, vx_set_##n); lbm_add_extension("canset-" name, vx_canset_##n);
    SET_LIST
#undef X

    // The display driver and hardware of the Dash, which only need to succeed
    const char *ok[] = {"import", "conf-store", "image-save", "disp-load-sh8501b", "disp-load-ili9341",
                        "disp-load-st7789", "disp-load-ssd1306", "disp-load-icna3306", "disp-orientation", "disp-set-bl", "btn-pull-en", "i2c-start"};
    for (unsigned int i = 0;i < sizeof(ok) / sizeof(ok[0]);i++) {
        lbm_add_extension(ok[i], vx_true);
    }

    return true;
}

static void vl_run(void) {
    // The reader and printer use strtod and printf, which follow the locale of
    // the app. Qt applies the locale of the user on Unix (not on Windows), so
    // numbers in the code would be read as 0 where the decimal separator is a
    // comma.
#ifndef _WIN32
    static locale_t c_locale;
    if (!c_locale) {
        c_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    }
    uselocale(c_locale);
#endif
    lbm_run_eval();
}

static void vl_kill(void) {
    lbm_kill_eval();
}

// Like lispif_restart in the firmware: define the imports, then evaluate the code
static bool vl_load(const char *code, int len) {
    if (!pause_eval()) {
        return false;
    }

    int chars = (int)strnlen(code, len);
    if (len > chars + 3) {
        int32_t ind = chars + 1;
        int num = buffer_get_uint16((uint8_t*)code, &ind);
        for (int i = 0;i < num && num < 500 && ind < len;i++) {
            char *name = (char*)code + ind;
            ind += (int32_t)strlen(name) + 1;
            int32_t offset = buffer_get_int32((uint8_t*)code, &ind);
            int32_t size = buffer_get_int32((uint8_t*)code, &ind);
            lbm_value val;
            if (offset >= 0 && size >= 0 && offset + size <= len &&
                    lbm_share_array_const(&val, (char*)code + offset, size)) {
                lbm_define(name, val);
            }
        }
    }

    lbm_create_string_char_channel(&tok_state, &tok, (char*)code);
    bool ok = lbm_load_and_eval_program_incremental(&tok, "main-u") > 0;
    lbm_continue_eval();
    return ok;
}

static bool vl_eval(const char *str, bool program) {
    // The last REPL command or stream is still being read
    if ((!program && repl_cid >= 0) || (program && stream_buf && !lbm_channel_is_empty(&stream_tok))) {
        return false;
    }

    if (!pause_eval()) {
        return false;
    }

    // Streamed code is read before it runs, so the REPL can be used while it runs
    lbm_cid cid;
    if (program) {
        free(stream_buf);
        stream_buf = strdup(str);
        lbm_create_string_char_channel(&stream_state, &stream_tok, stream_buf);
        cid = lbm_load_and_eval_program(&stream_tok, "main-s");
    } else {
        free(repl_buf);
        repl_buf = strdup(str);
        lbm_create_string_char_channel(&repl_state, &repl_tok, repl_buf);
        cid = repl_cid = lbm_load_and_eval_expression(&repl_tok);
    }
    lbm_continue_eval();
    return cid > 0;
}

static void vl_stats(float *heapUse, float *memUse, void *h, void (*global)(void *, const char *, double)) {
    *heapUse = lbm_heap_state.gc_num > 0 ?
                100.0f * (float)(HEAP_CELLS - lbm_heap_state.gc_last_free) / (float)HEAP_CELLS : 0.0f;
    *memUse = 100.0f * (float)(lbm_memory_num_words() - lbm_memory_num_free()) / (float)lbm_memory_num_words();

    if (!pause_eval()) {
        return;
    }

    lbm_value *env = lbm_get_global_env();
    for (int i = 0;i < GLOBAL_ENV_ROOTS;i++) {
        for (lbm_value c = env[i];lbm_is_cons(c);c = lbm_cdr(c)) {
            lbm_value kv = lbm_car(c);
            if (lbm_is_symbol(lbm_car(kv)) && lbm_is_number(lbm_cdr(kv))) {
                global(h, lbm_get_name_by_symbol(lbm_dec_sym(lbm_car(kv))), lbm_dec_as_double(lbm_cdr(kv)));
            }
        }
    }
    lbm_continue_eval();
}

static void vl_mock(const vesc_mock_state_t *state) {
    vesc_mock_write(state);
}

// Same values as lispif_process_can, lispif_process_custom_app_data and lispif_process_rmsg
static void vl_event(const char *what, int id, int arg, const uint8_t *data, int len) {
    lbm_flat_value_t v;

    if (strcmp(what, "rmsg") == 0) {
        if (arg < 0 || arg >= RMSG_SLOTS) {
            return;
        }

        lbm_mutex_lock(&rmsg_mutex);
        // A reply after the timeout of canmsg-recv is not for that call
        if (rmsg_timeout[arg] > 0 && lbm_timestamp() - rmsg_start[arg] > rmsg_timeout[arg]) {
            rmsg_cid[arg] = -1;
        }
        if (rmsg_cid[arg] >= 0 && lbm_start_flatten(&v, 10 + len)) {
            f_lbm_array(&v, len, (uint8_t*)data);
            lbm_finish_flatten(&v);
            if (lbm_unblock_ctx(rmsg_cid[arg], &v)) {
                rmsg_cid[arg] = -1;
            } else {
                lbm_free(v.buf);
            }
        }
        lbm_mutex_unlock(&rmsg_mutex);
        return;
    }

    bool rx = strcmp(what, "data-rx") == 0;
    lbm_uint sym = rx ? sym_data_rx : (strcmp(what, "can-sid") == 0 ? sym_can_sid : sym_can_eid);
    if (sym == 0 || !lbm_start_flatten(&v, 50 + len)) {
        return;
    }

    f_cons(&v);
    f_sym(&v, sym);
    if (!rx) {
        f_cons(&v);
        f_i32(&v, id);
    }
    f_lbm_array(&v, len, (uint8_t*)data);
    lbm_finish_flatten(&v);

    if (!lbm_event(&v)) {
        lbm_free(v.buf);
    }
}

const vl_api_t VL_NAME(api) = {vl_init, vl_run, vl_kill, vl_load, vl_eval, vl_stats, vl_mock, vl_event};
