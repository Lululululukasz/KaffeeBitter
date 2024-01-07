//
// Created by helen on 13.12.2023.
//
#include "globals.h"

QueueHandle_t scaleQueue = NULL;
QueueHandle_t apiQueue = NULL;

SemaphoreHandle_t apiMessage_handle = NULL;
int apiMessageLength = 0;
char* apiMessage = NULL;

SemaphoreHandle_t storage_handle = NULL;

bool timeConnected = false;

const char *getStateName(enum State state) {
    switch (state) {
        case noKettle:
            return "no";
        case emptyKettle:
            return "empty";
        case filledKettle:
            return "filled";
        default:
            return "Error";
    }
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
        case stateLoaded:
            return "stateLoaded";
        default:
            return "Error";
    }
}

const char* getTemperatureName(enum Temperature temperature) {
    switch (temperature) {
        case hot:
            return "hot";
        case warm:
            return "warm";
        case cold:
            return "cold";
        default:
            return "Error";
    }
}
