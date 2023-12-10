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
#include "credentials.h"

static const char *TAG = "hx711";

#define N 32
#define SCALE_ZERO_GRAMS -189250
#define SCALE_500_GRAMS -213525
#define SCALE_EMPTY_KETTLE_G 1825
#define MARGIN_OF_ERROR_G 100


static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
        case WIFI_EVENT_STA_START:
            printf("WiFi connecting WIFI_EVENT_STA_START ... \n");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            printf("WiFi connected WIFI_EVENT_STA_CONNECTED ... \n");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            printf("WiFi lost connection WIFI_EVENT_STA_DISCONNECTED ... \n");
            break;
        case IP_EVENT_STA_GOT_IP:
            printf("WiFi got IP ... \n\n");
            break;
        default:
            break;
    }
}

void wifi_connection()
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
            .sta = {
                    .ssid = SSID,
                    .password = PASS}};
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_connect();
}

static esp_err_t post_handler(httpd_req_t *req)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    httpd_resp_send(req, "URI POST Response ... from ESP32", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void server_initiation()
{
    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server_handle = NULL;
    httpd_start(&server_handle, &server_config);
    httpd_uri_t uri_post = {
            .uri = "/",
            .method = HTTP_POST,
            .handler = post_handler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server_handle, &uri_post);
}

void wifi(void *pvParameters) {
    wifi_connection();
    server_initiation();
    vTaskDelete(NULL);
}

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
void app_main()
{
    xTaskCreate(wifi, "wifi", configMINIMAL_STACK_SIZE * 5, NULL, 4, NULL);
    xTaskCreate(weight, "weight", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
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

