//
// Created by helen on 13.12.2023.
//

#ifndef HX711_DETERMINESTATE_H
#define HX711_DETERMINESTATE_H

#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hx711.h>
#include <stdio.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include <esp_netif_sntp.h>
#include "time.h"
#include "wifi.h"
#include "globals.h"
#include "weight.h"
#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "nvs_flash.h"
#include "time.h"
#include "globals.h"


int32_t difference(int32_t a, int32_t b);

void setDetailesState(struct DetailedData* res, enum State state, struct Measurement measurement);

int32_t calculateCupsFromWeight(int32_t weight);

bool checkForWeight(int32_t weight, int32_t ref);

void updateState(struct DetailedData* data, enum StateChange stateChange);

void determineState(void *pvParameters);

#endif //HX711_DETERMINESTATE_H
