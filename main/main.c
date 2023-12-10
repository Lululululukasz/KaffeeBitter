#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hx711.h>

#define N 32
#define SCALE_ZERO_GRAMS -189250
#define SCALE_500_GRAMS -213525
#define SCALE_EMPTY_KETTLE_G 1825
#define MARGIN_OF_ERROR_G 100

static const char *TAG = "hx711-example";

// core 1 for tasks, core 0 does wifi


TaskHandle_t weight_handle = NULL;
TaskHandle_t determineState_handle = NULL;
QueueHandle_t queue;

enum state {
    noKettle,
    emptyKettle,
    partiallyFullKettle,
    fullKettle,
};

int32_t difference(int32_t a, int32_t b);

int32_t inGrams(int32_t raw);

int32_t readRawScaleValue(hx711_t dev, int times);

void weight(void *pvParameters);

void determineState(void *pvParameters);

void app_main()
{
    queue = xQueueCreate(5, sizeof(int32_t));

    xTaskCreate(
            weight, // function
            "weight", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            5, // priority
            &weight_handle // task handle: interaction from within other tasks
    );

    xTaskCreate(
            determineState, // function
            "determineState", // name of task in debug messages
            configMINIMAL_STACK_SIZE * 5, // stack size
            NULL, // parameters
            5, // priority
            &determineState_handle // task handle: interaction from within other tasks
    );
}

int32_t difference(int32_t a, int32_t b) {
    if (a > b) {
        return a - b;
    } else {
        return b - a;
    }
}

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

        data = inGrams(data);
        ESP_LOGI(TAG, "Data in g: %" PRIi32, data);

        xQueueSend(queue, &data, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

bool checkForWeight(int32_t weight, int32_t ref) {
    return (difference(weight, ref) < MARGIN_OF_ERROR_G);
}

void determineState(void *pvParameters) {

    int32_t coffeeAmount = 0;

    while (1) {
        int32_t weight;
        xQueueReceive(queue, &weight, portMAX_DELAY);

        if (weight >= SCALE_EMPTY_KETTLE_G + MARGIN_OF_ERROR_G) {
            coffeeAmount = weight - SCALE_EMPTY_KETTLE_G;
            ESP_LOGI(TAG, "Amount of Coffee: %" PRIi32, coffeeAmount);
        } else if (checkForWeight(weight, SCALE_EMPTY_KETTLE_G)) {
            ESP_LOGI(TAG, "Empty Kettle: %" PRIi32, weight);
        } else {
            ESP_LOGI(TAG, "No Kettle: %" PRIi32, weight);
        }
    }

}

