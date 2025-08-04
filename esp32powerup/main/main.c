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

static void wifi_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
    ESP_LOGI(TAG, "Connecting to WiFi...");
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

    // Configure LED pin
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0); // LED off initially

    start_webserver();
}
