/* Copyright 2026 Espressif Systems (Shanghai) CO LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error codes returned by the esp-serial-flasher library.
 */
typedef enum {
    ESP_LOADER_SUCCESS,                /*!< Success */
    ESP_LOADER_ERROR_FAIL,             /*!< Unspecified error */
    ESP_LOADER_ERROR_TIMEOUT,          /*!< Timeout elapsed */
    ESP_LOADER_ERROR_IMAGE_SIZE,       /*!< Image size to flash is larger than flash size */
    ESP_LOADER_ERROR_INVALID_MD5,      /*!< Computed and received MD5 does not match */
    ESP_LOADER_ERROR_INVALID_PARAM,    /*!< Invalid parameter passed to function */
    ESP_LOADER_ERROR_INVALID_TARGET,   /*!< Connected target is invalid */
    ESP_LOADER_ERROR_UNSUPPORTED_CHIP, /*!< Attached chip is not supported */
    ESP_LOADER_ERROR_UNSUPPORTED_FUNC, /*!< Function is not supported on attached target */
    ESP_LOADER_ERROR_INVALID_RESPONSE  /*!< Internal error */
} esp_loader_error_t;

#ifdef __cplusplus
}
#endif
