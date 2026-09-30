#ifndef __APP_WIFI_H__
#define __APP_WIFI_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册 WiFi 调试命令到控制台（scan / join / disconnect / autoreconnect）
 *
 * WiFi 协议栈在命令首次使用时懒初始化。
 * 注意：NVS 已由 hw_board_init() 完成初始化，本模块无需再处理。
 */
void app_wifi_register_console(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_WIFI_H__ */
