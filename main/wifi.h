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

void wifi_connection();

static esp_err_t post_handler(httpd_req_t *req)
{
    xSemaphoreTake(apiMessage_handle, portMAX_DELAY);
    xSemaphoreTake(apiMessageLength_handle, portMAX_DELAY);
    httpd_resp_send(req, apiMessage, apiMessageLength);
    xSemaphoreGive(apiMessage_handle);
    xSemaphoreGive(apiMessageLength_handle);
    return ESP_OK;
}

void server_initiation();

void wifi(void *pvParameters);

void api(void *pvParameters);

esp_err_t initialize_time(void);


#endif //HX711_WIFI_H
