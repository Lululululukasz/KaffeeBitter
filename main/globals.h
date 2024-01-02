//
// Created by helen on 13.12.2023.
//

#ifndef HX711_GLOBALS_H
#define HX711_GLOBALS_H

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

#define N 32
#define SCALE_ZERO_GRAMS -178223
#define SCALE_500_GRAMS -153420
#define SCALE_EMPTY_KETTLE_G 1825
#define MARGIN_OF_ERROR_G 100
#define CUP_SIZE_ML 200

extern TaskHandle_t weight_handle;
extern TaskHandle_t determineState_handle;
extern TaskHandle_t api_handle;
extern QueueHandle_t scaleQueue;
extern QueueHandle_t apiQueue;
extern SemaphoreHandle_t apiMessage_handle;
extern int apiMessageLength;
extern char* apiMessage;

extern const char *TAG;

extern const char* ntpServer;

enum State {
    noKettle,
    emptyKettle,
    filledKettle
};

const char* getStateName(enum State state);

enum StateChange {
    noMoreKettle,
    coffeeEmpty,
    newEmptyKettle,
    lessCoffee,
    freshCoffee
};

const char* getStateChangeName(enum StateChange stateChange);

enum Temperature {
    hot,
    warm,
    cold
};

struct ExternalCoffeeData {
    enum State state;
    int32_t cupsOfCoffee;
    enum Temperature temperature;
};

struct Measurement {
    int32_t weightG;
    time_t timestamp;
};

const char* getTemperatureName(enum Temperature temperature);

struct DetailedData {
    enum State state;
    struct Measurement measurement;
    time_t freshCoffee;
    int32_t cupsOfCoffee;
    enum Temperature temperature;
    int32_t coffeeAmountMl;
};

#endif //HX711_GLOBALS_H
