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

// true if there is a file, false if not
bool check_for_save_file();

// loads the persistently saved data into the parameter data
void read_from_flash_memory(struct DetailedData *data);

// writes the parameter data into persistent file
void write_to_flash_memory(struct DetailedData *data);

// updates a changed state, gives the updated state to the api
void updateState(struct DetailedData *data, enum StateChange stateChange, struct Measurement measurement);

// determines the amount of cups of coffee in the kettle
int32_t calculateCupsFromWeight(int32_t weight);

// compares if the two weights are close enough to each other
bool checkForWeight(int32_t weight, int32_t ref);

// determines the apprximate temperature of the coffee
enum Temperature calculateCoffeeTemperature(time_t freshCoffee, time_t current);

void determineState(void *pvParameters) {
    const char *tag = "state(determine)";

    // initialise spiffs
    esp_vfs_spiffs_conf_t spiff_conf = {
            .base_path = "/storage",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&spiff_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(tag, "Failed to mount SPIFFS: %s", esp_err_to_name(ret));
    }

    // load data/create blank slate
    struct DetailedData currentData;
    if (check_for_save_file()) {
        read_from_flash_memory(&currentData);
        updateState(&currentData, stateLoaded, currentData.measurement);
    } else {
        currentData.state = noKettle;
        write_to_flash_memory(&currentData);

        struct ExternalCoffeeData webData = {
                .state = currentData.state,
                .cupsOfCoffee = currentData.cupsOfCoffee,
                .temperature = currentData.temperature,
        };

        xQueueSend(apiQueue, &webData, portMAX_DELAY);
    }

    while (1) {
        struct Measurement measurement;
        xQueueReceive(scaleQueue, &measurement, portMAX_DELAY);

        int32_t cups = calculateCupsFromWeight(measurement.weightG);

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
                if (cups >= currentData.cupsOfCoffee) {
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

bool check_for_save_file() {
    const char *tag = "state(checkSave)";

    xSemaphoreTake(storage_handle, portMAX_DELAY);
    FILE *file = fopen("/storage/data.txt", "r");

    if (file == NULL) {
        ESP_LOGE(tag, "No File");
        fclose(file);
        xSemaphoreGive(storage_handle);
        return false;
    } else {
        ESP_LOGE(tag, "File Found");
        fclose(file);
        xSemaphoreGive(storage_handle);
        return true;
    }
}

void read_from_flash_memory(struct DetailedData *data) {
    const char *tag = "state(load)";

    xSemaphoreTake(storage_handle, portMAX_DELAY);
    FILE *file = fopen("/storage/data.txt", "r");
    if (file == NULL) {
        ESP_LOGE(tag, "Failed to open file for reading");
        xSemaphoreGive(storage_handle);
        return;
    }

    fscanf(file,
           "{state: %u, measurement: {weightG: %ld, timestamp: %lld}, freshCoffee: %lld, cups: %ld, temp: %u, coffeeMl: %ld}",
           &data->state, &data->measurement.weightG, &data->measurement.timestamp, &data->freshCoffee,
           &data->cupsOfCoffee, &data->temperature, &data->coffeeAmountMl);

    ESP_LOGE(tag,
             "Loaded: {state: %d, measurement: {weightG: %ld, timestamp: %lld}, freshCoffee: %lld, cups: %ld, temp: %d, coffeeMl: %ld}",
             data->state, data->measurement.weightG, data->measurement.timestamp, data->freshCoffee, data->cupsOfCoffee,
             data->temperature, data->coffeeAmountMl);

    char buffer[20];
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", localtime(&data->freshCoffee));
    ESP_LOGI(tag, "Last fresh Coffee: %s", buffer);

    fclose(file);
    xSemaphoreGive(storage_handle);
}

void write_to_flash_memory(struct DetailedData *data) {
    const char *tag = "state(save)";

    xSemaphoreTake(storage_handle, portMAX_DELAY);
    FILE *file = fopen("/storage/data.txt", "w");
    if (file == NULL) {
        ESP_LOGE(tag, "Failed to open file for writing");
        xSemaphoreGive(storage_handle);
        return;
    }

    fprintf(file,
            "{state: %d, measurement: {weightG: %ld, timestamp: %lld}, freshCoffee: %lld, cups: %ld, temp: %d, coffeeMl: %ld}",
            data->state, data->measurement.weightG, data->measurement.timestamp, data->freshCoffee, data->cupsOfCoffee,
            data->temperature, data->coffeeAmountMl);
    ESP_LOGE(tag,
             "Saved: {state: %d, measurement: {weightG: %ld, timestamp: %lld}, freshCoffee: %lld, cups: %ld, temp: %d, coffeeMl: %ld}",
             data->state, data->measurement.weightG, data->measurement.timestamp, data->freshCoffee, data->cupsOfCoffee,
             data->temperature, data->coffeeAmountMl);
    fclose(file);

    char buffer[20];
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", localtime(&data->freshCoffee));
    ESP_LOGI(tag, "Last fresh Coffee: %s", buffer);

    xSemaphoreGive(storage_handle);
}

void updateState(struct DetailedData *data, enum StateChange stateChange, struct Measurement measurement) {
    const char *tag = "state(update)";

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

        case stateLoaded:
            break;

        default:
            ESP_LOGI(tag, "Could not update: not a valid state");
            return;
    }

    struct ExternalCoffeeData webData = {
            .state = data->state,
            .cupsOfCoffee = data->cupsOfCoffee,
            .temperature = data->temperature,
    };

    xQueueSend(apiQueue, &webData, portMAX_DELAY);
    ESP_LOGI(tag, "State change: %s", getStateChangeName(stateChange));
    write_to_flash_memory(data);
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

enum Temperature calculateCoffeeTemperature(time_t freshCoffee, time_t current) {
    double dif = difftime(freshCoffee, current);
    if (dif >= HOURS_UNTIL_COLD) {
        return cold;
    } else if (dif >= HOURS_UNTIL_WARM) {
        return warm;
    } else {
        return hot;
    }
}