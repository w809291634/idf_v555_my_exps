/*
 * WiFi 应用层：STA 连接 + 控制台调试命令
 *
 * 参考示例：examples/idf_v555_my_exps/p4_touch_lcd4_3_exp/examples/04_wifistation
 * 说明：ESP32-P4 无原生 WiFi 外设，WiFi 由 esp_wifi_remote + esp_hosted
 *       通过板载协处理器提供，对上层仍使用标准 esp_wifi API。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"

#define DBG_TAG           "app_wifi"
#define DBG_LVL           DBG_INFO
#include <mydbg.h>          // must after of DBG_LVL, DBG_TAG or other options

#define WIFI_JOIN_TIMEOUT_MS    (10000)     // join 默认超时
#define WIFI_SCAN_LIST_SIZE     (20)        // scan 最多列出多少个 AP
#define WIFI_MAXIMUM_RETRY      (5)         // 断线自动重连的最大次数

/* FreeRTOS 事件组：连接成功 / 连接失败 */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

static bool s_initialized = false;          // WiFi 协议栈是否已初始化
static bool s_auto_reconnect = true;        // 断线自动重连开关（autoreconnect 命令可改）
static int  s_retry_num = 0;

static const char *wifi_authmode_str(wifi_auth_mode_t mode)
{
    switch (mode) {
    case WIFI_AUTH_OPEN:                    return "OPEN";
    case WIFI_AUTH_WEP:                     return "WEP";
    case WIFI_AUTH_WPA_PSK:                 return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:                return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:            return "WPA_WPA2_PSK";
    case WIFI_AUTH_ENTERPRISE:              return "ENTERPRISE";
    case WIFI_AUTH_WPA3_PSK:                return "WPA3_PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:           return "WPA2_WPA3_PSK";
    case WIFI_AUTH_WPA3_ENTERPRISE:         return "WPA3_ENTERPRISE";
    case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:    return "WPA2_WPA3_ENTERPRISE";
    case WIFI_AUTH_WAPI_PSK:                return "WAPI_PSK";
    default:                                return "UNKNOWN";
    }
}

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

/* WiFi 协议栈初始化，重复调用只生效一次 */
static bool wifi_stack_init(void)
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
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        log_e("create default wifi sta netif failed");
        return false;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler, NULL, NULL));
    /* 不把 SSID/密码写入 NVS，避免上电自动连接上次的 AP */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_initialized = true;
    return true;
}

/* 连接 AP 并等待结果 */
static bool wifi_join(const char *ssid, const char *pass, int timeout_ms)
{
    if (!wifi_stack_init()) {
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

/** 'scan' 命令：扫描并列出附近 AP */
static int cmd_wifi_scan(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!wifi_stack_init()) {
        return 1;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    log_i("scanning ...");
    esp_err_t err = esp_wifi_scan_start(NULL, true);
    if (err != ESP_OK) {
        log_e("scan start failed: %s", esp_err_to_name(err));
        return 1;
    }

    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK) {
        log_e("get ap num failed: %s", esp_err_to_name(err));
        return 1;
    }

    wifi_ap_record_t *ap_list = calloc(WIFI_SCAN_LIST_SIZE, sizeof(wifi_ap_record_t));
    if (ap_list == NULL) {
        log_e("no memory for scan result");
        return 1;
    }
    uint16_t number = WIFI_SCAN_LIST_SIZE;
    err = esp_wifi_scan_get_ap_records(&number, ap_list);
    if (err != ESP_OK) {
        log_e("get ap records failed: %s", esp_err_to_name(err));
        free(ap_list);
        return 1;
    }

    printf("%-4s %-32s %-6s %-4s %s\r\n", "No.", "SSID", "RSSI", "CH", "AUTH");
    for (int i = 0; i < number; i++) {
        printf("%-4d %-32s %-6d %-4d %s\r\n",
               i + 1, (char *)ap_list[i].ssid, ap_list[i].rssi,
               ap_list[i].primary, wifi_authmode_str(ap_list[i].authmode));
    }
    printf("total %u AP(s)\r\n", ap_count);
    free(ap_list);
    return 0;
}

/** 'join' 命令参数 */
static struct {
    struct arg_int *timeout;
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} join_args;

/** 'join' 命令：以 STA 模式连接 AP */
static int cmd_wifi_join(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&join_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, join_args.end, argv[0]);
        return 1;
    }
    log_i("connecting to '%s'", join_args.ssid->sval[0]);

    /* 未指定超时时使用默认值 */
    if (join_args.timeout->count == 0) {
        join_args.timeout->ival[0] = WIFI_JOIN_TIMEOUT_MS;
    }

    bool connected = wifi_join(join_args.ssid->sval[0],
                               join_args.password->sval[0],
                               join_args.timeout->ival[0]);
    if (!connected) {
        log_w("connection timed out");
        return 1;
    }
    return 0;
}

/** 'disconnect' 命令：断开当前连接 */
static int cmd_wifi_disconnect(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!wifi_stack_init()) {
        return 1;
    }
    esp_err_t err = esp_wifi_disconnect();
    if (err == ESP_ERR_WIFI_NOT_CONNECT) {
        log_i("not connected");
        return 0;
    }
    if (err != ESP_OK) {
        log_e("disconnect failed: %s", esp_err_to_name(err));
        return 1;
    }
    log_i("disconnected");
    return 0;
}

/** 'autoreconnect' 命令参数 */
static struct {
    struct arg_str *state;
    struct arg_end *end;
} autoreconnect_args;

/** 'autoreconnect' 命令：开关断线自动重连 */
static int cmd_wifi_autoreconnect(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&autoreconnect_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, autoreconnect_args.end, argv[0]);
        return 1;
    }

    const char *state = autoreconnect_args.state->sval[0];
    if (strcmp(state, "on") == 0) {
        s_auto_reconnect = true;
    } else if (strcmp(state, "off") == 0) {
        s_auto_reconnect = false;
    } else {
        log_e("invalid argument '%s', please use on/off", state);
        return 1;
    }
    log_i("auto reconnect %s", s_auto_reconnect ? "on" : "off");
    return 0;
}

void app_wifi_register_console(void)
{
    join_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    join_args.end = arg_end(2);

    autoreconnect_args.state = arg_str1(NULL, NULL, "<on|off>", "Enable/disable auto reconnect");
    autoreconnect_args.end = arg_end(1);

    const esp_console_cmd_t scan_cmd = {
        .command = "scan",
        .help = "Scan and list nearby WiFi APs",
        .hint = NULL,
        .func = &cmd_wifi_scan,
    };
    const esp_console_cmd_t join_cmd = {
        .command = "join",
        .help = "Join WiFi AP as a station",
        .hint = NULL,
        .func = &cmd_wifi_join,
        .argtable = &join_args
    };
    const esp_console_cmd_t disconnect_cmd = {
        .command = "disconnect",
        .help = "Disconnect from the current WiFi AP",
        .hint = NULL,
        .func = &cmd_wifi_disconnect,
    };
    const esp_console_cmd_t autoreconnect_cmd = {
        .command = "autoreconnect",
        .help = "Enable/disable reconnect automatically after disconnected",
        .hint = NULL,
        .func = &cmd_wifi_autoreconnect,
        .argtable = &autoreconnect_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&scan_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&join_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&disconnect_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&autoreconnect_cmd));
}
