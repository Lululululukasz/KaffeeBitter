//
// Created by helen on 13.12.2023.
//

#include "determineState.h"

int32_t difference(int32_t a, int32_t b) {
    if (a > b) {
        return a - b;
    } else {
        return b - a;
    }
}

int32_t calculateCupsFromWeight(int32_t weight) {
    return (weight - SCALE_EMPTY_KETTLE_G) / CUP_SIZE_ML;
}

bool checkForWeight(int32_t weight, int32_t ref) {
    return (difference(weight, ref) < MARGIN_OF_ERROR_G);
}

enum Temperature calculateCoffeeTemperature(time_t freshCoffee, time_t current) {
    double dif = difftime(freshCoffee, current);
    if(dif >= 24) {
        return cold;
    } else if (dif >= 6) {
        return warm;
    } else {
        return hot;
    }
}

void updateState(struct DetailedData* data, enum StateChange stateChange, struct Measurement measurement) {
    switch (stateChange) {
        case noMoreKettle:
            data->state = noKettle;
            data->measurement = measurement;
            data->freshCoffee = 0;
            data->cupsOfCoffee = 0;
            data->temperature = 0;
            data->coffeeAmountMl = 0;
            break;
        case coffeeEmpty:
            data->state = emptyKettle;
            data->measurement = measurement;
            // fresh coffee timestamp stays same
            data->cupsOfCoffee = 0;
            data->temperature = 0;
            data->coffeeAmountMl = measurement.weightG - SCALE_EMPTY_KETTLE_G;
            break;
        case newEmptyKettle:
            data->state = emptyKettle;
            data->measurement = measurement;
            data->freshCoffee = 0;
            data->cupsOfCoffee = 0;
            data->temperature = 0;
            data->coffeeAmountMl = measurement.weightG - SCALE_EMPTY_KETTLE_G;
            break;
        case lessCoffee:
            data->state = filledKettle;
            data->measurement = measurement;
            // fresh coffee timestamp stays same
            data->cupsOfCoffee = calculateCupsFromWeight(measurement.weightG);
            data->temperature = calculateCoffeeTemperature(data->freshCoffee, data->measurement.timestamp);
            data->coffeeAmountMl = measurement.weightG - SCALE_EMPTY_KETTLE_G;
            break;
        case freshCoffee:
            data->state = filledKettle;
            data->measurement = measurement;
            data->freshCoffee = measurement.timestamp;
            data->cupsOfCoffee = calculateCupsFromWeight(measurement.weightG);
            data->temperature = calculateCoffeeTemperature(data->freshCoffee, data->measurement.timestamp);
            data->coffeeAmountMl = measurement.weightG - SCALE_EMPTY_KETTLE_G;
            break;
    }

    struct ExternalCoffeeData webData = {
            .state = data->state,
            .cupsOfCoffee = data->cupsOfCoffee,
            .temperature = data->temperature,
    };

    xQueueSend(apiQueue, &webData, portMAX_DELAY);
    ESP_LOGI(TAG, "State change: (%s)" PRIi32, getStateChangeName(stateChange));
}


void determineState(void *pvParameters) {

    struct DetailedData currentData;
    currentData.state = noKettle;

    while (1) {
        struct Measurement measurement;
        xQueueReceive(scaleQueue, &measurement, portMAX_DELAY);

        int32_t cups = calculateCupsFromWeight(measurement.weightG);
        ESP_LOGI(TAG, "Current state: (%s)" PRIi32, getStateName(currentData.state));
        ESP_LOGI(TAG, "Cups of Coffee: %" PRIi32, cups);


        switch (currentData.state) {
            case noKettle: {
                if (cups > 0) {
                    updateState(&currentData, freshCoffee, measurement); // ! Fresh Coffee
                } else if (checkForWeight(measurement.weightG, SCALE_EMPTY_KETTLE_G)) {
                    updateState(&currentData, newEmptyKettle, measurement);
                } // else no Kettle -> nothing changed
                break;
            }
            case emptyKettle:
                if (cups > 0) {
                    break; // filled Kettle -> makes no sense
                } else if (!checkForWeight(measurement.weightG, SCALE_EMPTY_KETTLE_G)) {
                    updateState(&currentData, noMoreKettle, measurement);
                } // else empty Kettle -> nothing changed
                break;
            case filledKettle:
                if (cups >= currentData.cupsOfCoffee ) {
                    break; // no change or someone is pressing on the scale -> ignore
                } else if (cups > 0) {
                    updateState(&currentData, lessCoffee, measurement);
                } else if (checkForWeight(measurement.weightG, SCALE_EMPTY_KETTLE_G)) {
                    updateState(&currentData, coffeeEmpty, measurement);
                } else {
                    updateState(&currentData, noMoreKettle, measurement);
                }
                break;
        }
    }
}
