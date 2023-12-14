#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hx711.h>
#include <stdio.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include <esp_netif_sntp.h>
#include "time.h"
#include "globals.h"
#include "weight.h"
#include "determineState.h"


// core 1 for tasks, core 0 does wifi

void app_main()
{
    scaleQueue = xQueueCreate(5, sizeof(struct Measurement));
    apiQueue = xQueueCreate(5, sizeof(struct ExternalCoffeeData));
    apiMessage_handle = xSemaphoreCreateMutex();
    apiMessageLength_handle = xSemaphoreCreateMutex();

    xTaskCreate(
            weight, // function
            "weight", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            2, // priority
            &weight_handle // task handle: interaction from within other tasks
    );

    xTaskCreate(
            determineState, // function
            "determineState", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            3, // priority
            &determineState_handle // task handle: interaction from within other tasks
    );

    xTaskCreate(
            api, // function
            "api", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            5, // priority
            &api_handle // task handle: interaction from within other tasks
    );


}
