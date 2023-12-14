//
// Created by helen on 13.12.2023.
//

#include "weight.h"

int32_t inGrams(int32_t raw) {
    int32_t gram = (SCALE_500_GRAMS - SCALE_ZERO_GRAMS)/500;

    return (raw-SCALE_ZERO_GRAMS)/gram;
}

int32_t readRawScaleValue(hx711_t dev, int times) {
    esp_err_t r = hx711_wait(&dev, 500);
    if (r != ESP_OK)
    {
        ESP_LOGE(TAG, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
        return 0;
    }

    int32_t data;

    r = hx711_read_average(&dev, times, &data);

    if (r != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
        return 0;
    }
    return data;
}

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
        int32_t data;

        data = readRawScaleValue(dev, CONFIG_EXAMPLE_AVG_TIMES);

        ESP_LOGI(TAG, "Raw data: %" PRIi32, data);

        struct Measurement measurement;

        measurement.weightG = inGrams(data);
        ESP_LOGI(TAG, "Data in g: %" PRIi32, measurement.weightG);

        measurement.timestamp = time(NULL);
        char buffer[20];
        strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", localtime(&measurement.timestamp));
        ESP_LOGI(TAG, "Current time is: %s", buffer);

        xQueueSend(scaleQueue, &measurement, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}