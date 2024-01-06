//
// Created by helen on 03.01.2024.
//

#include "api.h"
#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include "globals.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#define MAX_HTTP_OUTPUT_BUFFER 2048
#define MIN(x, y) ((x) < (y) ? (x) : (y))

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
            const char* tag = "api(httpEvent)";

    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(tag, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(tag, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(tag, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(tag, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(tag, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(tag, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(tag, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(tag, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(tag, "Last esp error code: 0x%x", err);
                ESP_LOGI(tag, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(tag, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

void api(void *pvParameters) {
        const char* tag = "api(task)";

	vTaskDelay(10000 / portTICK_PERIOD_MS);
    struct ExternalCoffeeData currentWebData;

    while (1) {
        xQueueReceive(apiQueue, &currentWebData, portMAX_DELAY);

        xSemaphoreTake(apiMessage_handle, portMAX_DELAY);
        free(apiMessage);
        apiMessageLength =
                snprintf(NULL, 0, "{\"state\": \"%s\", \"cups\": %d, \"temperature\": \"%s\"}", getStateName(currentWebData.state), (int) currentWebData.cupsOfCoffee,
                         getTemperatureName(currentWebData.temperature));
        apiMessage = malloc(apiMessageLength + 1);
        sprintf(apiMessage, "{\"state\": \"%s\", \"cups\": %d, \"temperature\": \"%s\"}", getStateName(currentWebData.state), (int) currentWebData.cupsOfCoffee,
                getTemperatureName(currentWebData.temperature));

        ESP_LOGI(tag, "api message: %s", apiMessage);
        xSemaphoreGive(apiMessage_handle);
	
	 char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

   	 esp_http_client_config_t config = {
        	.host = "api.fsie-kiel.de",
       		.path = "/coffee",
        	.event_handler = _http_event_handler,
		.transport_type = HTTP_TRANSPORT_OVER_SSL,
        	.user_data = local_response_buffer,       
        	.disable_auto_redirect = true,
    	};
    	esp_http_client_handle_t client = esp_http_client_init(&config);

   	esp_http_client_set_url(client, "https://api.fsie-kiel.de/coffee");
   	esp_http_client_set_method(client, HTTP_METHOD_POST);
   	esp_http_client_set_header(client, "Content-Type", "application/json");
    	esp_http_client_set_post_field(client, apiMessage, apiMessageLength);
    	esp_err_t err = esp_http_client_perform(client);
    	if (err == ESP_OK) {
        	ESP_LOGI(tag, "HTTP POST Status = %d, content_length = %"PRId64,
                	esp_http_client_get_status_code(client),
                	esp_http_client_get_content_length(client));
    	} else {
        	ESP_LOGE(tag, "HTTP POST request failed: %s", esp_err_to_name(err));
    	}
	esp_http_client_cleanup(client);
    }
}
