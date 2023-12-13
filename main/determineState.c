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

void setDetailesState(struct DetailedData* res, enum State state, struct Measurement measurement) {
    switch (state) {
        case noKettle:
            res->state = noKettle;
            //res->timestamp = measurement.timestamp;
            res->tempInC = 0;
            res->coffeeAmountMl = measurement.weightG;
            res->cupsOfCoffee = 0;
            return;
        case emptyKettle:
            break;
        case filledKettle:
            break;
    }

}

int32_t calculateCupsFromWeight(int32_t weight) {
    return (weight - SCALE_EMPTY_KETTLE_G) / CUP_SIZE_ML;
}

bool checkForWeight(int32_t weight, int32_t ref) {
    return (difference(weight, ref) < MARGIN_OF_ERROR_G);
}

void updateState(struct DetailedData* data, enum StateChange stateChange) {
    switch (stateChange) {
        case noMoreKettle:
            data->state = noKettle;
            break;
        case coffeeEmpty:
        case newEmptyKettle:
            data->state = emptyKettle;
            break;
        case lessCoffee:
        case freshCoffee:
            data->state = filledKettle;
            break;
    }
    ESP_LOGI(TAG, "State change: (%s)" PRIi32, getStateChangeName(stateChange));
}


void determineState(void *pvParameters) {

    struct DetailedData currentData;
    currentData.state = noKettle;

    while (1) {
        struct Measurement measurement;
        xQueueReceive(queue, &measurement, portMAX_DELAY);

        int32_t cups = calculateCupsFromWeight(measurement.weightG);
        ESP_LOGI(TAG, "Current state: (%s)" PRIi32, getStateName(currentData.state));
        ESP_LOGI(TAG, "Cups of Coffee: %" PRIi32, cups);


        switch (currentData.state) {
            case noKettle: {
                if (cups > 0) {
                    updateState(&currentData, freshCoffee); // ! Fresh Coffee
                } else if (checkForWeight(measurement.weightG, SCALE_EMPTY_KETTLE_G)) {
                    updateState(&currentData, newEmptyKettle);
                } // else no Kettle -> nothing changed
                break;
            }
            case emptyKettle:
                if (cups > 0) {
                    break; // filled Kettle -> makes no sense
                } else if (!checkForWeight(measurement.weightG, SCALE_EMPTY_KETTLE_G)) {
                    updateState(&currentData, noMoreKettle);
                } // else empty Kettle -> nothing changed
                break;
            case filledKettle:
                if (cups >= currentData.cupsOfCoffee ) {
                    break; // no change or someone is pressing on the scale -> ignore
                } else if (cups > 0) {
                    updateState(&currentData, lessCoffee);
                } else if (checkForWeight(measurement.weightG, SCALE_EMPTY_KETTLE_G)) {
                    updateState(&currentData, coffeeEmpty);
                } else {
                    updateState(&currentData, noMoreKettle);
                }
                break;
        }

        /*

        if (measurement.weightG < SCALE_EMPTY_KETTLE_G) {
            if (lastStateChange.state == noKettle) {
                continue;
            } else {
                setDetailesState(&lastStateChange, noKettle, measurement);

                char buffer[20];
                strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", localtime(&lastStateChange.timestamp));
                ESP_LOGI(TAG, "Empty Kettle: %s", buffer);
                // send message here somehow
            }
        } else if (calculateCupsFromWeight(measurement.weightG) == 0) {
            if (lastStateChange.state == noKettle) {
                continue;
            } else {
                setDetailesState(&lastStateChange, noKettle, measurement);
        }

        if (measurement.weightG >= SCALE_EMPTY_KETTLE_G + MARGIN_OF_ERROR_G) {
            coffeeAmount = measurement.weightG - SCALE_EMPTY_KETTLE_G;
            cupAmount = coffeeAmount/CUP_SIZE_ML;
            ESP_LOGI(TAG, "Amount of Coffee: %" PRIi32, coffeeAmount);
            ESP_LOGI(TAG, "Cups of Coffee: %" PRIi32, cupAmount);
        } else if (checkForWeight(measurement.weightG, SCALE_EMPTY_KETTLE_G)) {
            ESP_LOGI(TAG, "Empty Kettle: %" PRIi32, measurement.weightG);
        } else {
            ESP_LOGI(TAG, "No Kettle: %" PRIi32, measurement.weightG);
        }
         */
    }

}


