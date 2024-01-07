#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "globals.h"
#include "tasks/weight.h"
#include "tasks/determineState.h"
#include "tasks/wifi.h"
#include "tasks/api.h"


void app_main()
{
    scaleQueue = xQueueCreate(5, sizeof(struct Measurement));
    apiQueue = xQueueCreate(5, sizeof(struct ExternalCoffeeData));
    apiMessage_handle = xSemaphoreCreateMutex();
    storage_handle = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(
            wifi, // function
            "wifi", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            1, // priority
            NULL, // task handle: interaction from within other tasks
            0 // core
    );

    xTaskCreatePinnedToCore(
            api, // function
            "api", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            2, // priority
            NULL, // task handle: interaction from within other tasks
            0 // core
    );

    xTaskCreatePinnedToCore(
            weight, // function
            "weight", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            1, // priority
            NULL, // task handle: interaction from within other tasks
            1 // core
    );

    xTaskCreatePinnedToCore(
            determineState, // function
            "determineState", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            2, // priority
            NULL, // task handle: interaction from within other tasks
            1 // core
    );

}
