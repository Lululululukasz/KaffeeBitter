//
// Created by helen on 13.12.2023.
//

#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include <time.h>
#include "globals.h"
#include "determineState.h"
#include "esp_spiffs.h"

void write_to_flash_memory();
void read_from_flash_memory();
int32_t calculateCupsFromWeight(int32_t weight);
bool checkForWeight(int32_t weight, int32_t ref);
void updateState(struct DetailedData *data, enum StateChange stateChange, struct Measurement measurement);
enum Temperature calculateCoffeeTemperature(time_t freshCoffee, time_t current);

void determineState(void *pvParameters) {
    const char* tag = "state(determine)";

    //setup spiff
    esp_vfs_spiffs_conf_t spiff_conf = {
            .base_path = "/storage",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&spiff_conf);

    if (ret != ESP_OK) {
        ESP_LOGE(tag, "Failed to mount SPIFFS: %s", esp_err_to_name(ret));
    } else {
        write_to_flash_memory();
        read_from_flash_memory();
    }

    struct DetailedData currentData;
    currentData.state = noKettle;



    while (1) {
        struct Measurement measurement;
        xQueueReceive(scaleQueue, &measurement, portMAX_DELAY);

        int32_t cups = calculateCupsFromWeight(measurement.weightG);
        //ESP_LOGI(TAG, "Current state: (%s)" PRIi32, getStateName(currentData.state));
        //ESP_LOGI(TAG, "Cups of Coffee: %" PRIi32, cups);


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

    //esp_vfs_spiffs_unregister(NULL);
}

void write_to_flash_memory() {
    const char* tag = "state(save)";

    xSemaphoreTake(storage_handle, portMAX_DELAY);
    FILE* file = fopen("/storage/example.txt", "w");
    if (file == NULL) {
        ESP_LOGE(tag, "Failed to open file for writing");
        xSemaphoreGive(storage_handle);
        return;
    }

    fprintf(file, "Irgendwas");
    ESP_LOGE(tag, "Data saved!");

    fclose(file);
    xSemaphoreGive(storage_handle);
}

void read_from_flash_memory() {
    const char* tag = "state(load)";

    xSemaphoreTake(storage_handle, portMAX_DELAY);
    FILE* file = fopen("/storage/example.txt", "r");
    if (file == NULL) {
        ESP_LOGE(tag, "Failed to open file for reading");
        xSemaphoreGive(storage_handle);
        return;
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        ESP_LOGE(tag, "File Line: %s", buffer);
    }

    fclose(file);
    xSemaphoreGive(storage_handle);
}

int32_t calculateCupsFromWeight(int32_t weight) {
    return (weight - SCALE_EMPTY_KETTLE_G) / CUP_SIZE_ML;
}

bool checkForWeight(int32_t weight, int32_t ref) {
    if (weight > ref) {
        return (weight - ref) < MARGIN_OF_ERROR_G;
    } else {
        return (ref - weight) < MARGIN_OF_ERROR_G;
    }
}

void updateState(struct DetailedData* data, enum StateChange stateChange, struct Measurement measurement) {
    const char* tag = "state(update)";


    switch (stateChange) {
        case noMoreKettle:
            data->state = noKettle;
            data->measurement = measurement;
            data->freshCoffee = 0;
            data->cupsOfCoffee = 0;
            data->temperature = cold;
            data->coffeeAmountMl = 0;
            break;
        case coffeeEmpty:
            data->state = emptyKettle;
            data->measurement = measurement;
            // fresh coffee timestamp stays same
            data->cupsOfCoffee = 0;
            data->temperature = cold;
            data->coffeeAmountMl = measurement.weightG - SCALE_EMPTY_KETTLE_G;
            break;
        case newEmptyKettle:
            data->state = emptyKettle;
            data->measurement = measurement;
            data->freshCoffee = 0;
            data->cupsOfCoffee = 0;
            data->temperature = cold;
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
    ESP_LOGI(tag, "State change: (%s)" PRIi32, getStateChangeName(stateChange));
}

enum Temperature calculateCoffeeTemperature(time_t freshCoffee, time_t current) {
    double dif = difftime(freshCoffee, current);
    if(dif >= HOURS_UNTIL_COLD) {
        return cold;
    } else if (dif >= HOURS_UNTIL_WARM) {
        return warm;
    } else {
        return hot;
    }
}