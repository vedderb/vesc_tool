/*
    Copyright 2024 Benjamin Vedder

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

#include "lispif.h"
#include "lispbm.h"
#include "utils.h"

static uint8_t dec_color(lbm_uint arg) {
	int c = 0;
	if (lbm_type_of_functional(arg) == LBM_TYPE_FLOAT) {
		float tmp = lbm_dec_as_float(arg);
		if (tmp < 1.001) {
			tmp *= 255.0;
		}
		c = tmp;
	} else {
		c = lbm_dec_as_u32(arg);
	}

	if (c < 0) {
		c = 0;
	} else if (c > 255) {
		c = 255;
	}

	return c;
}

static lbm_value ext_color_make(lbm_value *args, lbm_uint argn) {
	if (argn != 3 && argn != 4) {
		lbm_set_error_reason((char*)lbm_error_str_num_args);
		return ENC_SYM_TERROR;
	}

	LBM_CHECK_NUMBER_ALL();

	uint8_t r = dec_color(args[0]);
	uint8_t g = dec_color(args[1]);
	uint8_t b = dec_color(args[2]);
	uint8_t w = 0;
	if (argn == 4) {
		w = dec_color(args[3]);
	}

	uint32_t color = 0;
	color |= ((uint32_t)w) << 24;
	color |= ((uint32_t)r) << 16;
	color |= ((uint32_t)g) << 8;
	color |= ((uint32_t)b) << 0;

	if (argn == 4) {
		return lbm_enc_u32(color);
	} else {
		return lbm_enc_i(color);
	}
}

static lbm_value ext_color_split(lbm_value *args, lbm_uint argn) {
	if (argn != 1 && argn != 2) {
		lbm_set_error_reason((char*)lbm_error_str_num_args);
		return ENC_SYM_TERROR;
	}

	LBM_CHECK_NUMBER_ALL();

	uint32_t color = lbm_dec_as_u32(args[0]);
	uint8_t w = (color >> 24) & 0xFF;
	uint8_t r = (color >> 16) & 0xFF;
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t b = color & 0xFF;

	int type = 0;
	if (argn == 2) {
		type = lbm_dec_as_u32(args[1]);
	}

	lbm_value color_data = ENC_SYM_NIL;

	switch (type) {
	case 0:
		color_data = lbm_cons(lbm_enc_i(b), color_data);
		color_data = lbm_cons(lbm_enc_i(g), color_data);
		color_data = lbm_cons(lbm_enc_i(r), color_data);
		break;

	case 1:
		color_data = lbm_cons(lbm_enc_i(w), color_data);
		color_data = lbm_cons(lbm_enc_i(b), color_data);
		color_data = lbm_cons(lbm_enc_i(g), color_data);
		color_data = lbm_cons(lbm_enc_i(r), color_data);
		break;

	case 2:
		color_data = lbm_cons(lbm_enc_float((float)b / 255.0), color_data);
		color_data = lbm_cons(lbm_enc_float((float)g / 255.0), color_data);
		color_data = lbm_cons(lbm_enc_float((float)r / 255.0), color_data);
		break;

	case 3:
		color_data = lbm_cons(lbm_enc_float((float)w / 255.0), color_data);
		color_data = lbm_cons(lbm_enc_float((float)b / 255.0), color_data);
		color_data = lbm_cons(lbm_enc_float((float)g / 255.0), color_data);
		color_data = lbm_cons(lbm_enc_float((float)r / 255.0), color_data);
		break;

	default:
		break;
	}

	return color_data;
}

static uint32_t color_mix(uint32_t color1, uint32_t color2, float ratio) {
	uint8_t w1 = (color1 >> 24) & 0xFF;
	uint8_t r1 = (color1 >> 16) & 0xFF;
	uint8_t g1 = (color1 >> 8) & 0xFF;
	uint8_t b1 = color1 & 0xFF;

	uint8_t w2 = (color2 >> 24) & 0xFF;
	uint8_t r2 = (color2 >> 16) & 0xFF;
	uint8_t g2 = (color2 >> 8) & 0xFF;
	uint8_t b2 = color2 & 0xFF;

	utils_truncate_number(&ratio, 0.0, 1.0);

	uint8_t w_res = (uint8_t)((float)w1 * (1.0 - ratio) + (float)w2 * ratio);
	uint8_t r_res = (uint8_t)((float)r1 * (1.0 - ratio) + (float)r2 * ratio);
	uint8_t g_res = (uint8_t)((float)g1 * (1.0 - ratio) + (float)g2 * ratio);
	uint8_t b_res = (uint8_t)((float)b1 * (1.0 - ratio) + (float)b2 * ratio);

	uint32_t color_res = 0;
	color_res |= ((uint32_t)w_res) << 24;
	color_res |= ((uint32_t)r_res) << 16;
	color_res |= ((uint32_t)g_res) << 8;
	color_res |= ((uint32_t)b_res) << 0;

	return color_res;
}

static lbm_value ext_color_mix(lbm_value *args, lbm_uint argn) {
	if (argn != 3) {
		lbm_set_error_reason((char*)lbm_error_str_num_args);
		return ENC_SYM_TERROR;
	}

	if (!lbm_is_number(args[1]) || !lbm_is_number(args[2]) ||
			(!lbm_is_number(args[0]) && !lbm_is_list(args[0]))) {
		lbm_set_error_reason((char*)lbm_error_str_incorrect_arg);
		return ENC_SYM_TERROR;
	}

	uint32_t color2 = lbm_dec_as_u32(args[1]);
	float ratio = lbm_dec_as_float(args[2]);

	if (lbm_is_number(args[0])) {
		return lbm_enc_u32(color_mix(lbm_dec_as_u32(args[0]), color2, ratio));
	} else {
		lbm_value color_data = ENC_SYM_NIL;

		int curr = args[0];

		while (lbm_is_cons(curr)) {
			lbm_value arg = lbm_car(curr);
			if (lbm_is_number(arg)) {
				color_data = lbm_cons(lbm_enc_u32(
						color_mix(lbm_dec_as_u32(arg), color2, ratio)), color_data);
			}

			curr = lbm_cdr(curr);
		}

		return lbm_list_destructive_reverse(color_data);
	}
}

static uint32_t color_add_sub(uint32_t color1, uint32_t color2, bool sub) {
	int16_t w1 = (color1 >> 24) & 0xFF;
	int16_t r1 = (color1 >> 16) & 0xFF;
	int16_t g1 = (color1 >> 8) & 0xFF;
	int16_t b1 = color1 & 0xFF;

	int16_t w2 = (color2 >> 24) & 0xFF;
	int16_t r2 = (color2 >> 16) & 0xFF;
	int16_t g2 = (color2 >> 8) & 0xFF;
	int16_t b2 = color2 & 0xFF;

	int16_t w_res = w1 + (sub ? -w2 : w2);
	int16_t r_res = r1 + (sub ? -r2 : r2);
	int16_t g_res = g1 + (sub ? -g2 : g2);
	int16_t b_res = b1 + (sub ? -b2 : b2);

	if (w_res < 0) w_res = 0;
	if (r_res < 0) r_res = 0;
	if (g_res < 0) g_res = 0;
	if (b_res < 0) b_res = 0;

	if (w_res > 255) w_res = 255;
	if (r_res > 255) r_res = 255;
	if (g_res > 255) g_res = 255;
	if (b_res > 255) b_res = 255;

	uint32_t color_res = 0;
	color_res |= ((uint32_t)w_res) << 24;
	color_res |= ((uint32_t)r_res) << 16;
	color_res |= ((uint32_t)g_res) << 8;
	color_res |= ((uint32_t)b_res) << 0;

	return color_res;
}

static lbm_value ext_color_add_sub(lbm_value *args, lbm_uint argn, bool sub) {
	if (argn != 2) {
		lbm_set_error_reason((char*)lbm_error_str_num_args);
		return ENC_SYM_TERROR;
	}

	if (!lbm_is_number(args[1]) ||
			(!lbm_is_number(args[0]) && !lbm_is_list(args[0]))) {
		lbm_set_error_reason((char*)lbm_error_str_incorrect_arg);
		return ENC_SYM_TERROR;
	}

	uint32_t color2 = lbm_dec_as_u32(args[1]);

	if (lbm_is_number(args[0])) {
		return lbm_enc_u32(color_add_sub(lbm_dec_as_u32(args[0]), color2, sub));
	} else {
		lbm_value color_data = ENC_SYM_NIL;

		int curr = args[0];

		while (lbm_is_cons(curr)) {
			lbm_value arg = lbm_car(curr);
			if (lbm_is_number(arg)) {
				color_data = lbm_cons(lbm_enc_u32(
						color_add_sub(lbm_dec_as_u32(arg), color2, sub)), color_data);
			}

			curr = lbm_cdr(curr);
		}

		return lbm_list_destructive_reverse(color_data);
	}
}

static lbm_value ext_color_add(lbm_value *args, lbm_uint argn) {
	return ext_color_add_sub(args, argn, false);
}

static lbm_value ext_color_sub(lbm_value *args, lbm_uint argn) {
	return ext_color_add_sub(args, argn, true);
}

static uint32_t color_scale(uint32_t color, float scale) {
	uint8_t w = (color >> 24) & 0xFF;
	uint8_t r = (color >> 16) & 0xFF;
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t b = color & 0xFF;

	float w_res = (float)w * scale;
	float r_res = (float)r * scale;
	float g_res = (float)g * scale;
	float b_res = (float)b * scale;

	utils_truncate_number(&w_res, 0.0, 255.0);
	utils_truncate_number(&r_res, 0.0, 255.0);
	utils_truncate_number(&g_res, 0.0, 255.0);
	utils_truncate_number(&b_res, 0.0, 255.0);

	uint32_t color_res = 0;
	color_res |= ((uint32_t)w_res) << 24;
	color_res |= ((uint32_t)r_res) << 16;
	color_res |= ((uint32_t)g_res) << 8;
	color_res |= ((uint32_t)b_res) << 0;

	return color_res;
}

static lbm_value ext_color_scale(lbm_value *args, lbm_uint argn) {
	if (argn != 2) {
		lbm_set_error_reason((char*)lbm_error_str_num_args);
		return ENC_SYM_TERROR;
	}

	if (!lbm_is_number(args[1]) ||
			(!lbm_is_number(args[0]) && !lbm_is_list(args[0]))) {
		lbm_set_error_reason((char*)lbm_error_str_incorrect_arg);
		return ENC_SYM_TERROR;
	}

	float scale = lbm_dec_as_float(args[1]);

	if (lbm_is_number(args[0])) {
		return lbm_enc_u32(color_scale(lbm_dec_as_u32(args[0]), scale));
	} else {
		lbm_value color_data = ENC_SYM_NIL;

		int curr = args[0];

		while (lbm_is_cons(curr)) {
			lbm_value arg = lbm_car(curr);
			if (lbm_is_number(arg)) {
				color_data = lbm_cons(lbm_enc_u32(
						color_scale(lbm_dec_as_u32(arg), scale)), color_data);
			}

			curr = lbm_cdr(curr);
		}

		return lbm_list_destructive_reverse(color_data);
	}
}

bool lbm_color_extensions_init(void) {
	bool res = true;
	res = res && lbm_add_extension("color-make", ext_color_make);
	res = res && lbm_add_extension("color-split", ext_color_split);
	res = res && lbm_add_extension("color-mix", ext_color_mix);
	res = res && lbm_add_extension("color-add", ext_color_add);
	res = res && lbm_add_extension("color-sub", ext_color_sub);
	res = res && lbm_add_extension("color-scale", ext_color_scale);
	return res;
}
