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
#include "utils/tinygfx.c"
#define indexed4_mask sft_indexed4_mask
#define indexed4_shift sft_indexed4_shift
#define indexed16_mask sft_indexed16_mask
#define indexed16_shift sft_indexed16_shift
#include "src/extensions/schrift.c"
#undef indexed4_mask
#undef indexed4_shift
#undef indexed16_mask
#undef indexed16_shift

#include "src/env.c"
#include "src/eval_cps.c"
#include "src/extensions.c"
#include "src/fundamental.c"
#include "src/heap.c"
#include "src/lbm_c_interop.c"
#include "src/lbm_channel.c"
#include "src/lbm_custom_type.c"
#include "src/lbm_defrag_mem.c"
#include "src/lbm_flat_value.c"
#include "src/lbm_image.c"
#include "src/lbm_memory.c"
#include "src/lbm_prof.c"
#include "src/lispbm.c"
#include "src/print.c"
#include "src/stack.c"
#include "src/symrepr.c"
#include "src/tokpar.c"
#include "src/extensions/array_extensions.c"
#include "src/extensions/display_extensions.c"
#include "src/extensions/lbm_dyn_lib.c"
#include "src/extensions/math_extensions.c"
#include "src/extensions/mutex_extensions.c"
#include "src/extensions/random_extensions.c"
#include "src/extensions/runtime_extensions.c"
#include "src/extensions/set_extensions.c"
#include "src/extensions/string_extensions.c"
#include "src/extensions/ttf_backend.c"
#include "src/extensions/ttf_extensions.c"
#include "utils/buffer.c"
#include "utils/tjpgd.c"
#include "repl/vesc_extension_stubs.c"
#include "express/lbm_color_extensions.c"
#include "vl_api.h"

#define HEAP_CELLS 16384
#define MEM_BLOCKS 8192
#define IMAGE_WORDS 65536
#define EXT_MAX 700
#define RMSG_SLOTS 5

// Shared by all runtimes
extern void vl_sleep_us(uint32_t us);

static const vl_host_t *fns;
static void *host;

static lbm_cons_t vl_heap[HEAP_CELLS];
static lbm_uint vl_memory[LBM_MEMORY_SIZE_BLOCKS_TO_WORDS(MEM_BLOCKS)];
static lbm_uint vl_bitmap[LBM_MEMORY_BITMAP_SIZE(MEM_BLOCKS)];
static uint32_t vl_image[IMAGE_WORDS];
static lbm_extension_t vl_extensions[EXT_MAX];

static lbm_string_channel_state_t tok_state;
static lbm_char_channel_t tok;
static char *eval_buf;
static volatile lbm_cid repl_cid = -1;
static volatile lbm_cid rmsg_cid[RMSG_SLOTS];
static volatile bool ev_can_sid, ev_can_eid, ev_data_rx;
static char print_prefix[50];
static int ads_channel;

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
    for (int i = 0;i < 2000 && lbm_get_eval_state() != EVAL_CPS_STATE_PAUSED;i++) {
        vl_sleep_us(1000);
    }
    return lbm_get_eval_state() == EVAL_CPS_STATE_PAUSED;
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

static lbm_value vx_set_print_prefix(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN(1);
    if (!lbm_is_array_r(args[0])) {
        return ENC_SYM_TERROR;
    }
    strncpy(print_prefix, lbm_dec_str(args[0]), sizeof(print_prefix) - 1);
    return ENC_SYM_TRUE;
}

// 10 kHz ticks, like the motor controller
static lbm_value vx_systime(lbm_value *args, lbm_uint argn) {
    (void)args; (void)argn;
    return lbm_enc_u32(lbm_timestamp() / 100);
}

static lbm_value vx_secs_since(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN_NUMBER(1);
    return lbm_enc_float((float)(uint32_t)(lbm_timestamp() / 100 - lbm_dec_as_u32(args[0])) / 10000.0f);
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
    bool en = argn == 1 || lbm_dec_as_i32(args[1]);
    const char *name = lbm_get_name_by_symbol(lbm_dec_sym(args[0]));
    if (strcmp(name, "event-can-sid") == 0) {
        ev_can_sid = en;
    } else if (strcmp(name, "event-can-eid") == 0) {
        ev_can_eid = en;
    } else if (strcmp(name, "event-data-rx") == 0) {
        ev_data_rx = en;
    }
    return ENC_SYM_TRUE;
}

// (send-data data) and (can-send-sid/eid id data)
static lbm_value vl_send(const char *what, int id, int arg, lbm_value data) {
    uint8_t buf[500];
    fns->send(host, what, id, arg, buf, to_bytes(data, buf, sizeof(buf)));
    return ENC_SYM_TRUE;
}

static lbm_value vx_send_data(lbm_value *args, lbm_uint argn) {
    return argn >= 1 ? vl_send("send-data", 0, 0, args[0]) : ENC_SYM_TERROR;
}

static lbm_value vx_can_send_sid(lbm_value *args, lbm_uint argn) {
    return argn == 2 ? vl_send("can-send-sid", lbm_dec_as_i32(args[0]), 0, args[1]) : ENC_SYM_TERROR;
}

static lbm_value vx_can_send_eid(lbm_value *args, lbm_uint argn) {
    return argn == 2 ? vl_send("can-send-eid", lbm_dec_as_i32(args[0]), 0, args[1]) : ENC_SYM_TERROR;
}

static lbm_value vx_canmsg_send(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN(3);
    return vl_send("canmsg-send", lbm_dec_as_i32(args[0]), lbm_dec_as_i32(args[1]), args[2]);
}

// (canmsg-recv slot timeout), blocks until canmsg-send to the slot or the timeout
static lbm_value vx_canmsg_recv(lbm_value *args, lbm_uint argn) {
    LBM_CHECK_ARGN_NUMBER(2);
    int slot = lbm_dec_as_i32(args[0]);
    float timeout = lbm_dec_as_float(args[1]);
    if (slot < 0 || slot >= RMSG_SLOTS) {
        return ENC_SYM_TERROR;
    }
    rmsg_cid[slot] = lbm_get_current_cid();
    if (timeout > 0.0f) {
        lbm_block_ctx_from_extension_timeout(timeout);
    } else {
        lbm_block_ctx_from_extension();
    }
    return ENC_SYM_TRUE;
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
    LBM_CHECK_ARGN_NUMBER(1); return vl_get(name, lbm_dec_as_i32(args[0])); }
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

// (set-current val) and (canset-current id val), the rest of the motor commands alike
static lbm_value vl_set(const char *what, lbm_value *args, lbm_uint argn) {
    if (argn > 2 || !lbm_is_number(args[argn - 1])) {
        return ENC_SYM_TERROR;
    }
    fns->set(host, what, argn == 2 ? lbm_dec_as_i32(args[0]) : -1, lbm_dec_as_double(args[argn - 1]));
    return ENC_SYM_TRUE;
}

#define SET_LIST \
    X(current, "current") X(current_rel, "current-rel") X(brake, "brake") X(brake_rel, "brake-rel") \
    X(duty, "duty") X(rpm, "rpm")
#define X(n, name) \
    static lbm_value vx_set_##n(lbm_value *args, lbm_uint argn) { \
    return argn == 1 ? vl_set(name, args, argn) : ENC_SYM_TERROR; } \
    static lbm_value vx_canset_##n(lbm_value *args, lbm_uint argn) { \
    return argn == 2 ? vl_set(name, args, argn) : ENC_SYM_TERROR; }
SET_LIST
#undef X

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
    if (!fns->conf(host, name, &val, set, def)) {
        return ENC_SYM_EERROR;
    }
    return set ? ENC_SYM_TRUE : lbm_enc_float((float)val);
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
            uint32_t c = getpixel(img, i, j);
            switch (img->fmt) {
            case indexed2: case indexed4: case indexed16:
                c = COLOR_TO_RGB888(colors[c], i, j);
                break;
            case rgb332:
                c = ((c & 0xE0) << 16) | ((c & 0x1C) << 11) | ((c & 0x03) << 6);
                break;
            case rgb565:
                c = ((c & 0xF800) << 8) | ((c & 0x07E0) << 5) | ((c & 0x001F) << 3);
                break;
            default:
                break;
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
    free(eval_buf);
    eval_buf = NULL;
    repl_cid = -1;
    ev_can_sid = ev_can_eid = ev_data_rx = false;
    print_prefix[0] = 0;
    for (int i = 0;i < RMSG_SLOTS;i++) {
        rmsg_cid[i] = -1;
    }

    if (!lbm_init(vl_heap, HEAP_CELLS, vl_memory, LBM_MEMORY_SIZE_BLOCKS_TO_WORDS(MEM_BLOCKS),
                  vl_bitmap, LBM_MEMORY_BITMAP_SIZE(MEM_BLOCKS), 256, 256, vl_extensions, EXT_MAX)) {
        return false;
    }

    lbm_set_usleep_callback(vl_sleep_us);
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
    lbm_add_extension("can-list-devs", vx_can_list_devs);
    lbm_add_extension("can-msg-age", vx_can_msg_age);
    lbm_add_extension("get-bms-val", vx_get_bms_val);
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
                        "disp-load-st7789", "disp-load-ssd1306", "disp-load-icna3306", "disp-reset",
                        "disp-orientation", "disp-set-bl", "btn-pull-en", "i2c-start"};
    for (unsigned int i = 0;i < sizeof(ok) / sizeof(ok[0]);i++) {
        lbm_add_extension(ok[i], vx_true);
    }

    return true;
}

static void vl_run(void) {
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
    if (!program && repl_cid >= 0) {
        return false;
    }

    if (!pause_eval()) {
        return false;
    }

    free(eval_buf);
    eval_buf = strdup(str);
    lbm_create_string_char_channel(&tok_state, &tok, eval_buf);
    lbm_cid cid = program ? lbm_load_and_eval_program_incremental(&tok, "main-s") :
                            lbm_load_and_eval_expression(&tok);
    if (!program) {
        repl_cid = cid;
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
    bool rmsg = strcmp(what, "rmsg") == 0;
    bool sid = strcmp(what, "can-sid") == 0;
    bool eid = strcmp(what, "can-eid") == 0;
    bool rx = strcmp(what, "data-rx") == 0;

    if ((rmsg && (arg < 0 || arg >= RMSG_SLOTS || rmsg_cid[arg] < 0)) ||
            (sid && !ev_can_sid) || (eid && !ev_can_eid) || (rx && !ev_data_rx)) {
        return;
    }

    lbm_uint sym;
    char name[20];
    snprintf(name, sizeof(name), "event-%s", what);
    if (!rmsg && !lbm_get_symbol_by_name(name, &sym)) {
        return;
    }

    lbm_flat_value_t v;
    if (!lbm_start_flatten(&v, 50 + len)) {
        return;
    }

    if (!rmsg) {
        f_cons(&v);
        f_sym(&v, sym);
        if (!rx) {
            f_cons(&v);
            f_i32(&v, id);
        }
    }
    f_lbm_array(&v, len, (uint8_t*)data);
    lbm_finish_flatten(&v);

    bool ok = rmsg ? lbm_unblock_ctx(rmsg_cid[arg], &v) : lbm_event(&v);
    if (rmsg && ok) {
        rmsg_cid[arg] = -1;
    }
    if (!ok) {
        lbm_free(v.buf);
    }
}

const vl_api_t VL_NAME(api) = {vl_init, vl_run, vl_kill, vl_load, vl_eval, vl_stats, vl_mock, vl_event};
