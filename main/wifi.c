#include "wifi.h"

httpd_handle_t start_webserver(void);

void stop_webserver(httpd_handle_t server);

static void
wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    switch (event_id) {
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

void wifi_connection() {
    ESP_LOGI(TAG, "Wifi Network: (%s)", CONFIG_WIFI_SSID);
    ESP_LOGI(TAG, "Wifi Password: (%s)", CONFIG_WIFI_PASSWORD);

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
                    .ssid = CONFIG_WIFI_SSID,
                    .password = CONFIG_WIFI_PASSWORD
            }
    };
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_connect();
}

void server_initiation() {
    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server_handle = NULL;
    httpd_start(&server_handle, &server_config);
    httpd_uri_t uri_post = {
            .uri = "/",
            .method = HTTP_POST,
            .handler = post_handler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server_handle, &uri_post);

    httpd_uri_t uri_post2 = {
            .uri = "/data",
            .method = HTTP_GET,
            .handler = post_handler,
            .user_ctx = NULL};
    httpd_register_uri_handler(server_handle, &uri_post2);
}

// wifi task
void wifi(void *pvParameters) {
    wifi_connection();
    server_initiation();
    ESP_ERROR_CHECK(initialize_time());

    vTaskDelete(NULL);
}

void api(void *pvParameters) {
    struct ExternalCoffeeData currentWebData;

    while (1) {
        xQueueReceive(apiQueue, &currentWebData, portMAX_DELAY);

        xSemaphoreTake(apiMessage_handle, portMAX_DELAY);
        xSemaphoreTake(apiMessageLength_handle, portMAX_DELAY);
        free(apiMessage);
        apiMessageLength =
                snprintf(NULL, 0, "{%s, %d, %s}", getStateName(currentWebData.state), (int) currentWebData.cupsOfCoffee,
                         getTemperatureName(currentWebData.temperature)) + 1;
        apiMessage = malloc(apiMessageLength);
        sprintf(apiMessage, "{%s, %d, %s}", getStateName(currentWebData.state), (int) currentWebData.cupsOfCoffee,
                getTemperatureName(currentWebData.temperature));

        ESP_LOGI(TAG, "api message: (%s)", apiMessage);
        xSemaphoreGive(apiMessage_handle);
        xSemaphoreGive(apiMessageLength_handle);

    }
}


esp_err_t initialize_time(void) {
    // SET SNTP
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);

    // set timezone
    setenv("TZ", "GMT-1", 1);
    tzset();

    // GET SNTP response
    esp_err_t response = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000));

    // Print time
    char buffer[20];
    time_t now = time(NULL);
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
    ESP_LOGI(TAG, "Current time is: %s", buffer);

    return response;
}

esp_err_t post_handler(httpd_req_t *req) {
    xSemaphoreTake(apiMessage_handle, portMAX_DELAY);
    xSemaphoreTake(apiMessageLength_handle, portMAX_DELAY);
    httpd_resp_send(req, apiMessage, apiMessageLength);
    ESP_LOGI(TAG, "api message: (%s)", apiMessage);
    xSemaphoreGive(apiMessage_handle);
    xSemaphoreGive(apiMessageLength_handle);
    return ESP_OK;
}