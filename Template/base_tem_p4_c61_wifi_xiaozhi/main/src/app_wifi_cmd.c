/*
 * WiFi console commands.
 *
 * The station core is implemented in app_wifi.c, this file only contains the
 * console front end: a single `wifi` command dispatched by subcommand.
 *
 *   wifi scan / join / disconnect / autoreconnect
 *   wifi status / stats / slave
 *   wifi ping / dns
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_console.h"
#include "esp_timer.h"
#include "argtable3/argtable3.h"
#include "esp_hosted.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "ping/ping_sock.h"
#include "app_wifi.h"

#define DBG_TAG           "app_wifi"
#define DBG_LVL           DBG_INFO
#include <mydbg.h>          // must after of DBG_LVL, DBG_TAG or other options

#define WIFI_JOIN_TIMEOUT_MS    (10000)     // default timeout of `wifi join`
#define WIFI_SCAN_LIST_SIZE     (20)        // max AP count listed by `wifi scan`
#define WIFI_PING_COUNT         (4)         // default count of `wifi ping`
#define WIFI_PING_TIMEOUT_MS    (1000)      // default per packet timeout of `wifi ping`

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

/** 'wifi scan' subcommand: scan and list nearby APs */
static int cmd_wifi_scan(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!app_wifi_stack_init()) {
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

/** 'wifi join' subcommand arguments */
static struct {
    struct arg_int *timeout;
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} join_args;

/** 'wifi join' subcommand: connect to an AP as station */
static int cmd_wifi_join(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&join_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, join_args.end, "wifi join");
        return 1;
    }
    log_i("connecting to '%s'", join_args.ssid->sval[0]);

    /* use the default timeout when it is not specified */
    if (join_args.timeout->count == 0) {
        join_args.timeout->ival[0] = WIFI_JOIN_TIMEOUT_MS;
    }

    bool connected = app_wifi_join(join_args.ssid->sval[0],
                                   join_args.password->sval[0],
                                   join_args.timeout->ival[0]);
    if (!connected) {
        log_w("connection timed out");
        return 1;
    }
    return 0;
}

/** 'wifi disconnect' subcommand: disconnect from the current AP */
static int cmd_wifi_disconnect(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!app_wifi_stack_init()) {
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

/** 'wifi autoreconnect' subcommand arguments */
static struct {
    struct arg_str *state;
    struct arg_end *end;
} autoreconnect_args;

/** 'wifi autoreconnect' subcommand: enable/disable auto reconnect */
static int cmd_wifi_autoreconnect(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&autoreconnect_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, autoreconnect_args.end, "wifi autoreconnect");
        return 1;
    }

    const char *state = autoreconnect_args.state->sval[0];
    if (strcmp(state, "on") == 0) {
        app_wifi_set_auto_reconnect(true);
    } else if (strcmp(state, "off") == 0) {
        app_wifi_set_auto_reconnect(false);
    } else {
        log_e("invalid argument '%s', please use on/off", state);
        return 1;
    }
    log_i("auto reconnect %s, saved in NVS", app_wifi_get_auto_reconnect() ? "on" : "off");
    return 0;
}

/** 'wifi status' subcommand: show link and IP status */
static int cmd_wifi_status(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!app_wifi_stack_init()) {
        return 1;
    }

    const char *mode_str = "UNKNOWN";
    wifi_mode_t mode = WIFI_MODE_NULL;
    if (esp_wifi_get_mode(&mode) == ESP_OK) {
        mode_str = (mode == WIFI_MODE_STA) ? "STA" :
                   (mode == WIFI_MODE_AP)  ? "AP"  : "STA+AP";
    }
    printf("Mode       : %s\r\n", mode_str);

    uint8_t mac[6] = { 0 };
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
        printf("STA MAC    : %02x:%02x:%02x:%02x:%02x:%02x\r\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    wifi_ap_record_t ap = { 0 };
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        printf("Link       : connected\r\n");
        printf("SSID       : %s\r\n", (char *)ap.ssid);
        printf("BSSID      : %02x:%02x:%02x:%02x:%02x:%02x\r\n",
               ap.bssid[0], ap.bssid[1], ap.bssid[2],
               ap.bssid[3], ap.bssid[4], ap.bssid[5]);
        printf("Channel    : %d\r\n", ap.primary);
        printf("RSSI       : %d dBm\r\n", ap.rssi);
        printf("Auth       : %s\r\n", wifi_authmode_str(ap.authmode));
    } else {
        printf("Link       : disconnected\r\n");
    }

    esp_netif_t *netif = app_wifi_sta_netif();
    if (netif != NULL) {
        esp_netif_ip_info_t ip = { 0 };
        if (esp_netif_get_ip_info(netif, &ip) == ESP_OK) {
            printf("IP         : " IPSTR "\r\n", IP2STR(&ip.ip));
            printf("Netmask    : " IPSTR "\r\n", IP2STR(&ip.netmask));
            printf("Gateway    : " IPSTR "\r\n", IP2STR(&ip.gw));
        }
        esp_netif_dns_info_t dns = { 0 };
        if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns) == ESP_OK) {
            printf("DNS        : " IPSTR "\r\n", IP2STR(&dns.ip.u_addr.ip4));
        }
    }

    printf("Auto reconn: %s\r\n", app_wifi_get_auto_reconnect() ? "on" : "off");
    return 0;
}

/** 'wifi stats' subcommand: dump WiFi statistic counters */
static int cmd_wifi_stats(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!app_wifi_stack_init()) {
        return 1;
    }
    esp_err_t err = esp_wifi_statis_dump(WIFI_STATIS_ALL);
    if (err != ESP_OK) {
        log_e("statistic dump not supported: %s", esp_err_to_name(err));
        return 1;
    }
    return 0;
}

/** 'wifi slave' subcommand: show co-processor version, or reset it */
static int cmd_wifi_slave(int argc, char **argv)
{
    if (argc >= 2) {
        if (strcmp(argv[1], "reset") != 0) {
            log_e("invalid argument '%s', please use 'reset' or nothing", argv[1]);
            return 1;
        }
        log_w("resetting co-processor ...");
        esp_err_t err = esp_hosted_slave_reset();
        if (err != ESP_OK) {
            log_e("reset co-processor failed: %s", esp_err_to_name(err));
            return 1;
        }
        log_i("co-processor reset done");
        return 0;
    }

    esp_hosted_coprocessor_fwver_t ver = { 0 };
    esp_err_t err = esp_hosted_get_coprocessor_fwversion(&ver);
    if (err != ESP_OK) {
        log_e("get co-processor version failed: %s", esp_err_to_name(err));
        return 1;
    }
    printf("Co-processor FW : %" PRIu32 ".%" PRIu32 ".%" PRIu32 "\r\n",
           ver.major1, ver.minor1, ver.patch1);
    return 0;
}

/* ---------- wifi ping ---------- */

/** 'wifi ping' subcommand arguments */
static struct {
    struct arg_int *count;
    struct arg_int *timeout;
    struct arg_str *host;
    struct arg_end *end;
} ping_args;

static void wifi_ping_on_success(esp_ping_handle_t hdl, void *args)
{
    (void)args;

    uint16_t seqno = 0;
    uint16_t ttl = 0;
    uint32_t recv_len = 0;
    uint32_t elapsed_ms = 0;
    ip_addr_t addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_ms, sizeof(elapsed_ms));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &addr, sizeof(addr));
    printf("%" PRIu32 " bytes from %s: icmp_seq=%u ttl=%u time=%" PRIu32 " ms\r\n",
           recv_len, ipaddr_ntoa(&addr), seqno, ttl, elapsed_ms);
}

static void wifi_ping_on_timeout(esp_ping_handle_t hdl, void *args)
{
    (void)args;

    uint16_t seqno = 0;
    ip_addr_t addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &addr, sizeof(addr));
    printf("From %s: icmp_seq=%u timeout\r\n", ipaddr_ntoa(&addr), seqno);
}

static void wifi_ping_on_end(esp_ping_handle_t hdl, void *args)
{
    (void)args;

    uint32_t transmitted = 0;
    uint32_t received = 0;
    uint32_t duration_ms = 0;
    uint32_t loss = 0;
    ip_addr_t addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &duration_ms, sizeof(duration_ms));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &addr, sizeof(addr));
    if (transmitted > 0) {
        loss = (uint32_t)((1.0f - ((float)received / transmitted)) * 100);
    }
    printf("--- %s ping statistics ---\r\n", ipaddr_ntoa(&addr));
    printf("%" PRIu32 " packets transmitted, %" PRIu32 " received, %" PRIu32
           "%% packet loss, time %" PRIu32 " ms\r\n",
           transmitted, received, loss, duration_ms);
    esp_ping_delete_session(hdl);
}

/** 'wifi ping' subcommand: ICMP echo to a host */
static int cmd_wifi_ping(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&ping_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, ping_args.end, "wifi ping");
        return 1;
    }
    if (!app_wifi_stack_init()) {
        return 1;
    }

    /* resolve the host name to an IPv4 address */
    struct addrinfo hints = { 0 };
    hints.ai_family = AF_INET;
    struct addrinfo *res = NULL;
    if (getaddrinfo(ping_args.host->sval[0], NULL, &hints, &res) != 0 || res == NULL) {
        log_e("unknown host '%s'", ping_args.host->sval[0]);
        return 1;
    }
    /* The type field must be initialized: esp_ping_new_session() branches on
       IP_IS_V4()/IP_IS_V6() to pick the socket family and the ICMP packet type */
    ip_addr_t target = { 0 };
    struct in_addr addr4 = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    inet_addr_to_ip4addr(ip_2_ip4(&target), &addr4);
    IP_SET_TYPE_VAL(target, IPADDR_TYPE_V4);
    freeaddrinfo(res);

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.count = (ping_args.count->count > 0) ? (uint32_t)ping_args.count->ival[0] : WIFI_PING_COUNT;
    config.timeout_ms = (ping_args.timeout->count > 0) ? (uint32_t)ping_args.timeout->ival[0] : WIFI_PING_TIMEOUT_MS;
    config.target_addr = target;

    esp_ping_callbacks_t cbs = {
        .cb_args = NULL,
        .on_ping_success = wifi_ping_on_success,
        .on_ping_timeout = wifi_ping_on_timeout,
        .on_ping_end = wifi_ping_on_end,
    };
    esp_ping_handle_t ping = NULL;
    esp_err_t err = esp_ping_new_session(&config, &cbs, &ping);
    if (err != ESP_OK) {
        log_e("create ping session failed: %s", esp_err_to_name(err));
        return 1;
    }

    printf("PING %s (%s): %" PRIu32 " data bytes\r\n",
           ping_args.host->sval[0], ipaddr_ntoa(&target), config.data_size);
    err = esp_ping_start(ping);
    if (err != ESP_OK) {
        log_e("start ping failed: %s", esp_err_to_name(err));
        esp_ping_delete_session(ping);
        return 1;
    }
    return 0;
}

/* ---------- wifi dns ---------- */

/** 'wifi dns' subcommand arguments */
static struct {
    struct arg_str *host;
    struct arg_end *end;
} dns_args;

/** 'wifi dns' subcommand: resolve a host name */
static int cmd_wifi_dns(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&dns_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, dns_args.end, "wifi dns");
        return 1;
    }
    if (!app_wifi_stack_init()) {
        return 1;
    }

    struct addrinfo hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *res = NULL;

    int64_t start_us = esp_timer_get_time();
    int err = getaddrinfo(dns_args.host->sval[0], NULL, &hints, &res);
    int64_t cost_ms = (esp_timer_get_time() - start_us) / 1000;
    if (err != 0 || res == NULL) {
        log_e("resolve '%s' failed, err=%d", dns_args.host->sval[0], err);
        return 1;
    }

    for (struct addrinfo *it = res; it != NULL; it = it->ai_next) {
        char ip[16] = { 0 };
        inet_ntoa_r(((struct sockaddr_in *)it->ai_addr)->sin_addr, ip, sizeof(ip));
        printf("%s -> %s\r\n", dns_args.host->sval[0], ip);
    }
    freeaddrinfo(res);
    printf("resolved in %lld ms\r\n", (long long)cost_ms);
    return 0;
}

/* 'wifi' command usage and examples, printed when the subcommand is missing */
static const char WIFI_USAGE[] =
    "Usage: wifi <subcommand> [args]\n"
    "  wifi scan                                        scan and list nearby APs\n"
    "  wifi join <ssid> [<pass>] [--timeout=<ms>]       join AP as station (default 10000 ms)\n"
    "  wifi disconnect                                  disconnect from the current AP\n"
    "  wifi autoreconnect <on|off>                      auto connect on boot, saved in NVS\n"
    "  wifi status                                      show link/IP/DNS status\n"
    "  wifi stats                                       dump WiFi statistic counters\n"
    "  wifi slave [reset]                               show co-processor version, or reset it\n"
    "  wifi ping <host> [--count=<n>] [--timeout=<ms>]  ICMP echo to a host\n"
    "  wifi dns <host>                                  resolve a host name\n"
    "Examples:\n"
    "  wifi scan\n"
    "  wifi join MyAP\n"
    "  wifi join MyAP 12345678\n"
    "  wifi join MyAP 12345678 --timeout=20000\n"
    "  wifi status\n"
    "  wifi stats\n"
    "  wifi ping 8.8.8.8\n"
    "  wifi ping www.espressif.com --count=10 --timeout=2000\n"
    "  wifi dns www.espressif.com\n"
    "  wifi slave\n"
    "  wifi slave reset\n"
    "  wifi disconnect\n"
    "  wifi autoreconnect off\n";

/** 'wifi' command: entry of all WiFi debug operations, selected by subcommand */
static int cmd_wifi(int argc, char **argv)
{
    if (argc < 2) {
        printf("%s", WIFI_USAGE);
        return 1;
    }

    /* shift the subcommand name to argv[0] (argtable3 skips argv[0]) */
    const char *sub = argv[1];
    if (strcmp(sub, "scan") == 0) {
        return cmd_wifi_scan(argc - 1, argv + 1);
    } else if (strcmp(sub, "join") == 0) {
        return cmd_wifi_join(argc - 1, argv + 1);
    } else if (strcmp(sub, "disconnect") == 0) {
        return cmd_wifi_disconnect(argc - 1, argv + 1);
    } else if (strcmp(sub, "autoreconnect") == 0) {
        return cmd_wifi_autoreconnect(argc - 1, argv + 1);
    } else if (strcmp(sub, "status") == 0) {
        return cmd_wifi_status(argc - 1, argv + 1);
    } else if (strcmp(sub, "stats") == 0) {
        return cmd_wifi_stats(argc - 1, argv + 1);
    } else if (strcmp(sub, "slave") == 0) {
        return cmd_wifi_slave(argc - 1, argv + 1);
    } else if (strcmp(sub, "ping") == 0) {
        return cmd_wifi_ping(argc - 1, argv + 1);
    } else if (strcmp(sub, "dns") == 0) {
        return cmd_wifi_dns(argc - 1, argv + 1);
    }

    log_e("unknown subcommand '%s'", sub);
    printf("%s", WIFI_USAGE);
    return 1;
}

void app_wifi_register_console(void)
{
    join_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    join_args.end = arg_end(2);

    autoreconnect_args.state = arg_str1(NULL, NULL, "<on|off>", "Enable/disable auto reconnect");
    autoreconnect_args.end = arg_end(1);

    ping_args.count = arg_int0(NULL, "count", "<n>", "Stop after sending count packets");
    ping_args.timeout = arg_int0(NULL, "timeout", "<t>", "Timeout of each packet, ms");
    ping_args.host = arg_str1(NULL, NULL, "<host>", "Host address or name");
    ping_args.end = arg_end(2);

    dns_args.host = arg_str1(NULL, NULL, "<host>", "Host name to resolve");
    dns_args.end = arg_end(1);

    const esp_console_cmd_t wifi_cmd = {
        .command = "wifi",
        .help = "WiFi debug commands, use a subcommand to select the operation\n"
                "  wifi scan\n"
                "  wifi join <ssid> [<pass>] [--timeout=<ms>]\n"
                "  wifi disconnect\n"
                "  wifi autoreconnect <on|off>\n"
                "  wifi status / stats / slave [reset]\n"
                "  wifi ping <host> [--count=<n>] [--timeout=<ms>]\n"
                "  wifi dns <host>\n"
                "Examples:\n"
                "  wifi join MyAP 12345678 --timeout=20000\n"
                "  wifi ping www.espressif.com --count=10\n"
                "  wifi dns www.espressif.com",
        .hint = "<scan|join|disconnect|autoreconnect|status|stats|slave|ping|dns> [args]",
        .func = &cmd_wifi,
        /* no argtable here: each subcommand parses its own arguments */
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_cmd));

    /* top level alias, so that 'ping <host>' works without the 'wifi' prefix */
    const esp_console_cmd_t ping_cmd = {
        .command = "ping",
        .help = "ICMP echo to a host, same as 'wifi ping'",
        .hint = "<host> [--count=<n>] [--timeout=<ms>]",
        .func = &cmd_wifi_ping,
        /* argv passed to the function has the same shape as 'wifi ping ...' */
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&ping_cmd));
}
