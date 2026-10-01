/*
 * Bridge between the template entry point and the xiaozhi application.
 *
 * xiaozhi's own main/main.cc is not built: this project keeps src/main.c as the
 * single entry point so that hw_board_init() and the console stay in charge of
 * the boot sequence. This file does the two things that main.cc did.
 */
#include <esp_err.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "application.h"        // from the referenced xiaozhi-esp32 tree

#define TAG "app_xiaozhi"

extern "C" void app_xiaozhi_start(void)
{
    // Initialize NVS flash for WiFi configuration
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize and run the application
    auto& app = Application::GetInstance();
    app.Initialize();
    app.Run();  // This function runs the main event loop and never returns
}
