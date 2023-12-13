//
// Created by helen on 13.12.2023.
//

#ifndef HX711_WIFI_H
#define HX711_WIFI_H

#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include <esp_netif_sntp.h>
#include "time.h"
#include "globals.h"

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
void wifi_connection();

static esp_err_t post_handler(httpd_req_t *req)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    httpd_resp_send(req, "URI POST Response ... from ESP32", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void server_initiation();

void wifi(void *pvParameters);

esp_err_t initialize_time(void);

#endif //HX711_WIFI_H
