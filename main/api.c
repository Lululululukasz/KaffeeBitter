//
// Created by helen on 03.01.2024.
//

#include "api.h"
#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include "globals.h"

void api(void *pvParameters) {
    struct ExternalCoffeeData currentWebData;

    while (1) {
        xQueueReceive(apiQueue, &currentWebData, portMAX_DELAY);

        xSemaphoreTake(apiMessage_handle, portMAX_DELAY);
        free(apiMessage);
        apiMessageLength =
                snprintf(NULL, 0, "{%s, %d, %s}", getStateName(currentWebData.state), (int) currentWebData.cupsOfCoffee,
                         getTemperatureName(currentWebData.temperature)) + 1;
        apiMessage = malloc(apiMessageLength);
        sprintf(apiMessage, "{%s, %d, %s}", getStateName(currentWebData.state), (int) currentWebData.cupsOfCoffee,
                getTemperatureName(currentWebData.temperature));

        ESP_LOGI(TAG, "api message: %s", apiMessage);
        xSemaphoreGive(apiMessage_handle);

    }
}