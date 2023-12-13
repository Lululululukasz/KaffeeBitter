//
// Created by helen on 13.12.2023.
//

#ifndef HX711_WEIGHT_H
#define HX711_WEIGHT_H

#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hx711.h>
#include <stdio.h>
#include "nvs_flash.h"
#include "time.h"
#include "globals.h"

int32_t inGrams(int32_t raw);

int32_t readRawScaleValue(hx711_t dev, int times);

void weight(void *pvParameters);

#endif //HX711_WEIGHT_H
