/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/soc_caps.h"
#include "cmd_system.h"
#include "cmd_wifi.h"
#include "cmd_nvs.h"
#include "console_settings.h"
#include "board_config.h"

/*
 * We warn if a secondary serial console is enabled. A secondary serial console is always output-only and
 * hence not very useful for interactive console applications. If you encounter this warning, consider disabling
 * the secondary serial console in menuconfig unless you know what you are doing.
 */
#if SOC_USB_SERIAL_JTAG_SUPPORTED
#if !CONFIG_ESP_CONSOLE_SECONDARY_NONE
#warning "A secondary serial console is not useful when using the console component. Please disable it in menuconfig."
#endif
#endif

static const char* TAG = "example";
#define PROMPT_STR CONFIG_IDF_TARGET

/* Console command history can be stored to and loaded from a file.
 * The easiest way to do this is to use FATFS filesystem on top of
 * wear_levelling library.
 */
#if CONFIG_CONSOLE_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#else
#define HISTORY_PATH NULL
#endif // CONFIG_CONSOLE_STORE_HISTORY

static void apl_console_task(void *pvParameter)
{
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char *prompt = setup_prompt(PROMPT_STR ">");

    ESP_LOGI(TAG, "Running on CPU %d", xPortGetCoreID());
    /* Main loop */
    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);

#if CONFIG_CONSOLE_IGNORE_EMPTY_LINES
        if (line == NULL) { /* Ignore empty lines */
            continue;;
        }
#else
        if (line == NULL) { /* Break on EOF or error */
            break;
        }
#endif // CONFIG_CONSOLE_IGNORE_EMPTY_LINES

        /* Add the command to the history if not empty*/
        if (strlen(line) > 0) {
            linenoiseHistoryAdd(line);
#if CONFIG_CONSOLE_STORE_HISTORY
            /* Save command history to filesystem */
            linenoiseHistorySave(HISTORY_PATH);
#endif // CONFIG_CONSOLE_STORE_HISTORY
        }

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }

    ESP_LOGE(TAG, "Error or end-of-input, terminating console");
    esp_console_deinit();
    vTaskDelete(NULL);
}

void apl_console_init(void)
{
#if CONFIG_CONSOLE_STORE_HISTORY
    // 需要 建立分区storage，用来建立文件系统，这里默认关闭掉
    // 官方示例默认打开
    initialize_filesystem();
    ESP_LOGI(TAG, "Command history enabled");
#else
    ESP_LOGI(TAG, "Command history disabled");
#endif

    /* Initialize console output periheral (UART, USB_OTG, USB_JTAG) */
    initialize_console_peripheral();

    /* Initialize linenoise library and esp_console*/
    initialize_console_library(HISTORY_PATH);

    /* Register commands */
    esp_console_register_help_command();
#if (!defined(BOARD_CONFIG_ENABLE_SYSTEM_CMD) || BOARD_CONFIG_ENABLE_SYSTEM_CMD)
    register_system_common();
#endif
#if SOC_LIGHT_SLEEP_SUPPORTED && (!defined(BOARD_CONFIG_ENABLE_SLEEP_CMD) || BOARD_CONFIG_ENABLE_SLEEP_CMD)
    register_system_light_sleep();
#endif
#if SOC_DEEP_SLEEP_SUPPORTED && (!defined(BOARD_CONFIG_ENABLE_SLEEP_CMD) || BOARD_CONFIG_ENABLE_SLEEP_CMD)
    register_system_deep_sleep();
#endif
#if (CONFIG_ESP_WIFI_ENABLED || CONFIG_ESP_HOST_WIFI_ENABLED) && (!defined(BOARD_CONFIG_ENABLE_WIFI_CMD) || BOARD_CONFIG_ENABLE_WIFI_CMD)
    register_wifi();
#endif
#if (!defined(BOARD_CONFIG_ENABLE_NVS_CMD) || BOARD_CONFIG_ENABLE_NVS_CMD)
    register_nvs();
#endif

    printf("\n"
           "This is an example of ESP-IDF console component.\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n"
           "Ctrl+C will terminate the console environment.\n");

    if (linenoiseIsDumbMode()) {
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Windows Terminal or Putty instead.\n");
    }

    // 创建任务（栈内存位置由 board_config 配置：1=PSRAM，0=内部 RAM）
#if BOARD_CONFIG_CONSOLE_TASK_STACK_IN_PSRAM
    const uint32_t task_stack_caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
#else
    const uint32_t task_stack_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
#endif
    BaseType_t task_created = xTaskCreatePinnedToCoreWithCaps(&apl_console_task, "apl_console",
        BOARD_CONFIG_CONSOLE_TASK_STACK_SIZE, NULL, BOARD_CONFIG_CONSOLE_TASK_PRIOR, NULL, BOARD_CONFIG_CONSOLE_TASK_CPU, task_stack_caps);
    ESP_ERROR_CHECK(task_created == pdPASS ? ESP_OK : ESP_FAIL);
}

