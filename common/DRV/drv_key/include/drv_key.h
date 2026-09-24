#pragma once
#include "board.h"
#include <stdint.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 按键宏 */
#if defined(BOARD_CONFIG_HAL_KEY0_GPIO) && defined(BOARD_CONFIG_HAL_KEY0_ACTIVATE)
  #define BORAD_HAL_KEY0                gpio_get_level(BOARD_CONFIG_HAL_KEY0_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY0_ACTIVATE  0
  #define BORAD_HAL_KEY0                (!BOARD_CONFIG_HAL_KEY0_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY1_GPIO) && defined(BOARD_CONFIG_HAL_KEY1_ACTIVATE)
  #define BORAD_HAL_KEY1                gpio_get_level(BOARD_CONFIG_HAL_KEY1_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY1_ACTIVATE  0
  #define BORAD_HAL_KEY1                (!BOARD_CONFIG_HAL_KEY1_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY2_GPIO) && defined(BOARD_CONFIG_HAL_KEY2_ACTIVATE)
  #define BORAD_HAL_KEY2                gpio_get_level(BOARD_CONFIG_HAL_KEY2_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY2_ACTIVATE  0
  #define BORAD_HAL_KEY2                (!BOARD_CONFIG_HAL_KEY2_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY3_GPIO) && defined(BOARD_CONFIG_HAL_KEY3_ACTIVATE)
  #define BORAD_HAL_KEY3                gpio_get_level(BOARD_CONFIG_HAL_KEY3_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY3_ACTIVATE  0
  #define BORAD_HAL_KEY3                (!BOARD_CONFIG_HAL_KEY3_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY4_GPIO) && defined(BOARD_CONFIG_HAL_KEY4_ACTIVATE)
  #define BORAD_HAL_KEY4                gpio_get_level(BOARD_CONFIG_HAL_KEY4_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY4_ACTIVATE  0
  #define BORAD_HAL_KEY4                (!BOARD_CONFIG_HAL_KEY4_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY5_GPIO) && defined(BOARD_CONFIG_HAL_KEY5_ACTIVATE)
  #define BORAD_HAL_KEY5                gpio_get_level(BOARD_CONFIG_HAL_KEY5_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY5_ACTIVATE  0
  #define BORAD_HAL_KEY5                (!BOARD_CONFIG_HAL_KEY5_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY6_GPIO) && defined(BOARD_CONFIG_HAL_KEY6_ACTIVATE)
  #define BORAD_HAL_KEY6                gpio_get_level(BOARD_CONFIG_HAL_KEY6_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY6_ACTIVATE  0
  #define BORAD_HAL_KEY6                (!BOARD_CONFIG_HAL_KEY6_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY7_GPIO) && defined(BOARD_CONFIG_HAL_KEY7_ACTIVATE)
  #define BORAD_HAL_KEY7                gpio_get_level(BOARD_CONFIG_HAL_KEY7_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY7_ACTIVATE  0
  #define BORAD_HAL_KEY7                (!BOARD_CONFIG_HAL_KEY7_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY8_GPIO) && defined(BOARD_CONFIG_HAL_KEY8_ACTIVATE)
  #define BORAD_HAL_KEY8                gpio_get_level(BOARD_CONFIG_HAL_KEY8_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY8_ACTIVATE  0
  #define BORAD_HAL_KEY8                (!BOARD_CONFIG_HAL_KEY8_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY9_GPIO) && defined(BOARD_CONFIG_HAL_KEY9_ACTIVATE)
  #define BORAD_HAL_KEY9                gpio_get_level(BOARD_CONFIG_HAL_KEY9_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY9_ACTIVATE  0
  #define BORAD_HAL_KEY9                (!BOARD_CONFIG_HAL_KEY9_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY10_GPIO) && defined(BOARD_CONFIG_HAL_KEY10_ACTIVATE)
  #define BORAD_HAL_KEY10                gpio_get_level(BOARD_CONFIG_HAL_KEY10_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY10_ACTIVATE  0
  #define BORAD_HAL_KEY10                (!BOARD_CONFIG_HAL_KEY10_ACTIVATE)
#endif

#if defined(BOARD_CONFIG_HAL_KEY11_GPIO) && defined(BOARD_CONFIG_HAL_KEY11_ACTIVATE)
  #define BORAD_HAL_KEY11                gpio_get_level(BOARD_CONFIG_HAL_KEY11_GPIO)
#else
  #define BOARD_CONFIG_HAL_KEY11_ACTIVATE  0
  #define BORAD_HAL_KEY11                (!BOARD_CONFIG_HAL_KEY11_ACTIVATE)
#endif

#define delay_ms(x)           vTaskDelay(x / portTICK_PERIOD_MS);

/* 按键位 */
#define KEYS_BITS(x)    ((uint32_t)((uint32_t)1 << x))
#define KEY0_PREESED      KEYS_BITS(0)
#define KEY1_PREESED      KEYS_BITS(1)
#define KEY2_PREESED      KEYS_BITS(2)
#define KEY3_PREESED      KEYS_BITS(3)
#define KEY4_PREESED      KEYS_BITS(4)
#define KEY5_PREESED      KEYS_BITS(5)
#define KEY6_PREESED      KEYS_BITS(6)
#define KEY7_PREESED      KEYS_BITS(7)
#define KEY8_PREESED      KEYS_BITS(8)
#define KEY9_PREESED      KEYS_BITS(9)
#define KEY10_PREESED     KEYS_BITS(10)
#define KEY11_PREESED     KEYS_BITS(11)

/* 函数 */
void key_pin_init(void);
unsigned int key_getState(void);
gpio_num_t KEY_Scan(uint8_t mode);
void test_key_getState_fun(void);
void test_KEY_Scan_fun(void);

#ifdef __cplusplus
}
#endif