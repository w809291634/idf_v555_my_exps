#ifndef __APP_WIFI_H__
#define __APP_WIFI_H__

#include <stdbool.h>
#include "esp_netif.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the WiFi driver in station mode (lazy, safe to call repeatedly)
 *
 * @return true on success
 */
bool app_wifi_stack_init(void);

/**
 * @brief Connect to an AP and wait for the result
 *
 * @param ssid        SSID of the AP
 * @param pass        PSK of the AP, NULL or "" for an open AP
 * @param timeout_ms  wait timeout in ms, <= 0 means wait forever
 *
 * @return true when connected
 */
bool app_wifi_join(const char *ssid, const char *pass, int timeout_ms);

/**
 * @brief Get the station netif handle, NULL before app_wifi_stack_init()
 */
esp_netif_t *app_wifi_sta_netif(void);

/**
 * @brief Connect back to the last AP at boot, using the SSID/password saved in NVS
 *
 * Call once from app_init(). When auto connect is disabled the WiFi stack is left
 * down, so that no radio current is drawn until the first console command.
 *
 * @return true when a connection attempt was started
 */
bool app_wifi_autoconnect_start(void);

/**
 * @brief Enable/disable auto connect, the setting is saved in NVS
 *
 * 'on'  : connect back to the last AP on the next boot
 * 'off' : keep the WiFi stack down until a console command is used
 */
void app_wifi_set_auto_reconnect(bool enable);

/**
 * @brief Get the auto connect setting
 */
bool app_wifi_get_auto_reconnect(void);

/**
 * @brief Register the WiFi debug command to the console
 *
 * Only one `wifi` command is registered, the operation is selected by a subcommand:
 *   wifi scan
 *   wifi join <ssid> [<pass>] [--timeout=<ms>]
 *   wifi disconnect
 *   wifi autoreconnect <on|off>
 *   wifi status
 *   wifi stats
 *   wifi slave [reset]
 *   wifi ping <host> [--count=<n>] [--timeout=<ms>]
 *   wifi dns <host>
 *
 * The WiFi stack is initialized lazily on the first use of a command, or at boot
 * by app_wifi_autoconnect_start() when auto connect is enabled.
 * Note: NVS is already initialized by hw_board_init(), no need to handle it here.
 */
void app_wifi_register_console(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_WIFI_H__ */
