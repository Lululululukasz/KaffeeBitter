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
        ESP_LOGI(TAG, "Data in g: %" PRIi32, inGrams(data));

        if(difference(inGrams(data), SCALE_EMPTY_KETTLE_G) < MARGIN_OF_ERROR_G) {
            ESP_LOGI(TAG, "Empty Kettle: %" PRIi32, inGrams(data));
        } else if (difference(inGrams(data), SCALE_EMPTY_KETTLE_G + 2000) < MARGIN_OF_ERROR_G) {
            ESP_LOGI(TAG, "Full Kettle: %" PRIi32, inGrams(data));
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main()
{
    xTaskCreate(weight, "weight", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}
