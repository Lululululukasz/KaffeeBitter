#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "globals.h"
#include "weight.h"
#include "determineState.h"
#include "api.h"

// core 1 for tasks, core 0 does wifi

void app_main()
{
    scaleQueue = xQueueCreate(5, sizeof(struct Measurement));
    apiQueue = xQueueCreate(5, sizeof(struct ExternalCoffeeData));
    apiMessage_handle = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(
            wifi, // function
            "wifi", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            2, // priority
            &weight_handle, // task handle: interaction from within other tasks
            0
    );

    xTaskCreatePinnedToCore(
            weight, // function
            "weight", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            2, // priority
            &weight_handle, // task handle: interaction from within other tasks
            1
    );

    xTaskCreatePinnedToCore(
            determineState, // function
            "determineState", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            3, // priority
            &determineState_handle, // task handle: interaction from within other tasks
            1
    );

    xTaskCreatePinnedToCore(
            api, // function
            "api", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            4, // priority
            &api_handle, // task handle: interaction from within other tasks
            0
    );


}
