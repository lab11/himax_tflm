/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#if defined(ARDUINO)
#define ARDUINO_EXCLUDE_CODE
#endif  // defined(ARDUINO)

#ifndef ARDUINO_EXCLUDE_CODE

#include "tensorflow/lite/micro/examples/person_detection/detection_responder.h"

#include "hx_drv_tflm.h"  // NOLINT

extern hx_drv_sensor_image_config_t g_pimg_config;

unsigned int crc32b(uint8_t * data, size_t len) {
   int j;
   unsigned int byte, crc, mask;

   crc = 0xFFFFFFFF;
   for (size_t i = 0; i < len; i++) {
     byte = data[i];                // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
   }
   return ~crc;
}

// This dummy implementation writes person and no person scores to the error
// console. Real applications will want to take some custom action instead, and
// should implement their own versions of this function.
void RespondToDetection(tflite::ErrorReporter* error_reporter,
                        int8_t person_score, int8_t no_person_score) {
  bool person_thresh = person_score > 40;

  if (person_thresh) {
    hx_drv_led_on(HX_DRV_LED_GREEN);
  } else {
    hx_drv_led_off(HX_DRV_LED_GREEN);
  }

  TF_LITE_REPORT_ERROR(error_reporter, "person score:%d no person score %d",
                       person_score, no_person_score);
  TF_LITE_REPORT_ERROR(error_reporter, "jpeg addr: %x jpeg size %d", (uint8_t*) g_pimg_config.jpeg_address, g_pimg_config.jpeg_size);


  if (person_thresh) {
    uint32_t crc = crc32b((uint8_t *) g_pimg_config.jpeg_address, g_pimg_config.jpeg_size);
    TF_LITE_REPORT_ERROR(error_reporter, "crc: %x", crc);

#define HX_I2C_MTU 128
    uint8_t addr = 0x57;
    HX_DRV_ERROR_E e = hx_drv_i2cm_set_data(120, &addr, 1, (uint8_t*) &g_pimg_config.jpeg_size, sizeof(g_pimg_config.jpeg_size));
    for (size_t i = 0; i < g_pimg_config.jpeg_size; i+=HX_I2C_MTU) {
      size_t len = HX_I2C_MTU;
      if (g_pimg_config.jpeg_size - i < HX_I2C_MTU) {
        len = g_pimg_config.jpeg_size - i;
      }
      addr = 0x58;
      e = hx_drv_i2cm_set_data(120, &addr, 1, (uint8_t *) g_pimg_config.jpeg_address + i, len);
      if (e) {
        TF_LITE_REPORT_ERROR(error_reporter, "i2c error jpeg: %d", e);
      }
    }
    //HX_DRV_ERROR_E e = hx_drv_i2cm_set_data(120, 0x00, 0, g_pimg_config.jpeg_address, 128);
    //e = hx_drv_i2cm_set_data(120, 0x01, 0, &no_person_score, 1);
    //TF_LITE_REPORT_ERROR(error_reporter, "i2c error nps: %d", e);
  }
}

#endif  // ARDUINO_EXCLUDE_CODE
