#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_http_server.h"

#include "secrets.h"

static const char *TAG = "LED_SERVER";
static bool led_on = false;

#define LED_GPIO GPIO_NUM_23

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_scan_and_print(void) {
    uint16_t number = 10;
    wifi_ap_record_t ap_info[10];
    uint16_t ap_count = 0;

    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Found %d access points:", ap_count);

    for (int i = 0; i < ap_count; i++) {
        ESP_LOGI(TAG, "SSID: %s, RSSI: %d, Authmode: %d",
                 ap_info[i].ssid, ap_info[i].rssi, ap_info[i].authmode);
    }
}



static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        wifi_scan_and_print();
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Retrying WiFi connection...");
        wifi_scan_and_print();
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void) {
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi...");

    /* Wait for connection */
    xEventGroupWaitBits(wifi_event_group,
                        WIFI_CONNECTED_BIT,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY);
}

// Handle GET /
esp_err_t root_get_handler(httpd_req_t *req) {
    const char *html_response =
        "<!DOCTYPE html><html><body>"
        "<h1>ESP32 LED Toggle</h1>"
        "<form action=\"/toggle\" method=\"post\">"
        "<button type=\"submit\">Toggle LED</button>"
        "</form></body></html>";

    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handle POST /toggle
esp_err_t toggle_post_handler(httpd_req_t *req) {
    led_on = !led_on;
    gpio_set_level(LED_GPIO, led_on ? 1 : 0);
    ESP_LOGI(TAG, "LED %s", led_on ? "ON" : "OFF");

    // Redirect back to root
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
        };
        httpd_uri_t toggle = {
            .uri = "/toggle",
            .method = HTTP_POST,
            .handler = toggle_post_handler,
        };
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &toggle);
    }
    return server;
}

void app_main(void) {
    nvs_flash_init();
    wifi_init();

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    start_webserver();
}
