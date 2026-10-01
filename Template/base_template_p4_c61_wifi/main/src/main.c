#include <stdio.h>
#include <inttypes.h>
/*
 * board.h is qualified on purpose: the referenced xiaozhi tree ships its own
 * boards/common/board.h (C++), which would otherwise shadow this project's
 * components/board_config/board.h (C). For the same reason apl_console.h and
 * drv_led.h are NOT included here - both pull in "board.h" as well:
 *   - the console is started by hw_board_init(),
 *   - led_pin_init() is a no-op on this board (BOARD_CONFIG_HAL_LED0_GPIO is unset).
 */
#include "components/board_config/board.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "apl_utility.h"

#define DBG_TAG           "main"
//#define DBG_LVL           DBG_INFO
#define DBG_LVL           DBG_LOG
//#define DBG_LVL           DBG_NODBG
#include <mydbg.h>          // must after of DBG_LVL, DBG_TAG or other options

/* Implemented in src/app_xiaozhi.cc, starts the xiaozhi voice assistant */
extern void app_xiaozhi_start(void);

void app_init(void)
{
    app_info_dump();
    app_xiaozhi_start();
}

void app_main(void)
{
    hw_board_init();
    app_init();
    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
