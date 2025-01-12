//
// Created by helen on 13.12.2023.
//

#include "weight.h"
#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hx711.h>
#include <stdio.h>
#include "nvs_flash.h"
#include "time.h"
#include "globals.h"
#include "sdkconfig.h"

// converts raw read into grams
int32_t inGrams(int32_t raw);

// reads raw measurement from the scale
int32_t readRawScaleValue(hx711_t dev, int times);

void weight(void *pvParameters) {

    hx711_t dev = {
            .dout = CONFIG_EXAMPLE_DOUT_GPIO,
            .pd_sck = CONFIG_EXAMPLE_PD_SCK_GPIO,
            .gain = HX711_GAIN_A_64
    };

    // initialize device
    ESP_ERROR_CHECK(hx711_init(&dev));

    // read from device
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));

        int32_t data;
        data = readRawScaleValue(dev, CONFIG_EXAMPLE_AVG_TIMES);

        struct Measurement measurement;
        measurement.weightG = inGrams(data);

        if(!timeConnected) { // wait for sntp to be set up before creating timestamps
            continue;
        }
        measurement.timestamp = time(NULL);

        xQueueSend(scaleQueue, &measurement, portMAX_DELAY);
    }
}

int32_t inGrams(int32_t raw) {
    int32_t gram = (CONFIG_SCALE_500_GRAMS - CONFIG_SCALE_ZERO_GRAMS)/500;
    return (raw - CONFIG_SCALE_ZERO_GRAMS)/gram;
}

int32_t readRawScaleValue(hx711_t dev, int times) {
    const char* tag = "weight(read)";

    esp_err_t r = hx711_wait(&dev, 500);
    if (r != ESP_OK) {
        ESP_LOGE(tag, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
        return 0;
    }

    int32_t data;
    r = hx711_read_average(&dev, times, &data);

    if (r != ESP_OK) {
        ESP_LOGE(tag, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
        return 0;
    }
    return data;
}