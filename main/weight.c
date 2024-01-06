//
// Created by helen on 13.12.2023.
//

#include "weight.h"


int32_t inGrams(int32_t raw) {
    int32_t gram = (SCALE_500_GRAMS - SCALE_ZERO_GRAMS)/500;

    return (raw-SCALE_ZERO_GRAMS)/gram;
}

int32_t readRawScaleValue(hx711_t dev, int times) {
    const char* tag = "weight(read)";

    esp_err_t r = hx711_wait(&dev, 500);
    if (r != ESP_OK)
    {
        ESP_LOGE(tag, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
        return 0;
    }

    int32_t data;

    r = hx711_read_average(&dev, times, &data);

    if (r != ESP_OK)
    {
        ESP_LOGE(tag, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
        return 0;
    }
    return data;
}

void weight(void *pvParameters) {
    //const char* tag = "weight(task)";

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
        vTaskDelay(pdMS_TO_TICKS(1000));
        int32_t data;

        data = readRawScaleValue(dev, CONFIG_EXAMPLE_AVG_TIMES);

        //ESP_LOGI(tag, "Raw data: %" PRIi32, data);

        struct Measurement measurement;

        measurement.weightG = inGrams(data);
        //ESP_LOGI(tag, "Data in g: %" PRIi32, measurement.weightG);

        if(!timeConnected) {
            continue;
        }
        measurement.timestamp = time(NULL);
        char buffer[20];
        strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", localtime(&measurement.timestamp));
        //ESP_LOGI(tag, "Current time is: %s", buffer);

        xQueueSend(scaleQueue, &measurement, portMAX_DELAY);
    }
}