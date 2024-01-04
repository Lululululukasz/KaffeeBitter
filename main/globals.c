//
// Created by helen on 13.12.2023.
//
#include "globals.h"

TaskHandle_t weight_handle = NULL;
TaskHandle_t determineState_handle = NULL;
TaskHandle_t api_handle = NULL;
QueueHandle_t scaleQueue = NULL;
QueueHandle_t apiQueue = NULL;
SemaphoreHandle_t apiMessage_handle = NULL;
int apiMessageLength = 0;
char* apiMessage = NULL;

const char* TAG = "hx711";

const char* ntpServer = "pool.ntp.org";

const char *getStateName(enum State state) {
    switch (state) {
        case noKettle:
            return "noKettle";
        case emptyKettle:
            return "emptyKettle";
        case filledKettle:
            return "filledKettle";
    }
    return "Error";
}

const char *getStateChangeName(enum StateChange stateChange) {
    switch (stateChange) {
        case noMoreKettle:
            return "noMoreKettle";
        case coffeeEmpty:
            return "coffeeEmpty";
        case newEmptyKettle:
            return "newEmptyKettle";
        case lessCoffee:
            return "lessCoffee";
        case freshCoffee:
            return "freshCoffee";
    }
    return "Error";
}

const char* getTemperatureName(enum Temperature temperature) {
    switch (temperature) {
        case hot:
            return "hot";
        case warm:
            return "warm";
        case cold:
            return "cold";
    }
    return "Error";
}