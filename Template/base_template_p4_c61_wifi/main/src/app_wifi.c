/*
 * WiFi application layer: station connection core.
 *
 * Reference example: examples/idf_v555_my_exps/p4_touch_lcd4_3_exp/examples/04_wifistation
 * Note: ESP32-P4 has no native WiFi peripheral. WiFi is provided by the onboard
 *       co-processor (SDIO) through esp_wifi_remote + esp_hosted, while the
 *       application layer still uses the standard esp_wifi API.
 *
 * The console commands are implemented in app_wifi_cmd.c.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "app_wifi.h"

#define DBG_TAG           "app_wifi"
#define DBG_LVL           DBG_INFO
#include <mydbg.h>          // must after of DBG_LVL, DBG_TAG or other options

#define WIFI_MAXIMUM_RETRY      (5)         // max retry count of auto reconnect

/* FreeRTOS event group: connected / failed */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

static bool s_initialized = false;          // is the WiFi stack initialized
static bool s_auto_reconnect = true;        // auto reconnect switch
static int  s_retry_num = 0;
static esp_netif_t *s_sta_netif = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        if (s_auto_reconnect && s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            log_i("retry to connect to the AP (%d/%d)", s_retry_num, WIFI_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            log_w("connect to the AP fail");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        log_i("got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

bool app_wifi_stack_init(void)
{
    if (s_initialized) {
        return true;
    }

    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        log_e("create event group failed");
        return false;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (s_sta_netif == NULL) {
        log_e("create default wifi sta netif failed");
        return false;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler, NULL, NULL));
    /* Do not store SSID/password in NVS, so that the device does not reconnect
       to the previous AP automatically after a reboot. */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_initialized = true;
    return true;
}

bool app_wifi_join(const char *ssid, const char *pass, int timeout_ms)
{
    if (ssid == NULL || ssid[0] == '\0') {
        log_e("ssid must not be empty");
        return false;
    }
    if (!app_wifi_stack_init()) {
        return false;
    }

    wifi_config_t wifi_config = { 0 };
    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (pass) {
        strlcpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    }

    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    s_retry_num = 0;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        log_e("connect failed: %s", esp_err_to_name(err));
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE,
                                           timeout_ms > 0 ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        log_i("connected to ap SSID:%s", ssid);
        return true;
    }
    log_w("failed to connect to SSID:%s", ssid);
    return false;
}

esp_netif_t *app_wifi_sta_netif(void)
{
    return s_sta_netif;
}

void app_wifi_set_auto_reconnect(bool enable)
{
    s_auto_reconnect = enable;
}

bool app_wifi_get_auto_reconnect(void)
{
    return s_auto_reconnect;
}
