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

esp_err_t post_handler(httpd_req_t *req);

void server_initiation();

void wifi(void *pvParameters);

void api(void *pvParameters);

esp_err_t initialize_time(void);


#endif //HX711_WIFI_H
