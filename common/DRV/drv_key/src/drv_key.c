#include <stdio.h>
#include "drv_key.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "apl_utility.h"

#define DBG_TAG           "drv_key"
//#define DBG_LVL           DBG_INFO
#define DBG_LVL           DBG_LOG
//#define DBG_LVL           DBG_NODBG
#include <mydbg.h>          // must after of DBG_LVL, DBG_TAG or other options

/*********************************************************************************************
* 名称：key_pin_init()
* 功能：KEY引脚初始化
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void key_pin_init(void)
{
#if defined(BOARD_CONFIG_HAL_KEY0_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY0_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY0_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY0_GPIO, BOARD_CONFIG_HAL_KEY0_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY1_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY1_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY1_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY1_GPIO, BOARD_CONFIG_HAL_KEY1_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY2_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY2_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY2_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY2_GPIO, BOARD_CONFIG_HAL_KEY2_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY3_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY3_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY3_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY3_GPIO, BOARD_CONFIG_HAL_KEY3_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY4_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY4_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY4_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY4_GPIO, BOARD_CONFIG_HAL_KEY4_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY5_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY5_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY5_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY5_GPIO, BOARD_CONFIG_HAL_KEY5_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY6_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY6_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY6_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY6_GPIO, BOARD_CONFIG_HAL_KEY6_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY7_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY7_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY7_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY7_GPIO, BOARD_CONFIG_HAL_KEY7_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY8_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY8_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY8_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY8_GPIO, BOARD_CONFIG_HAL_KEY8_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY9_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY9_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY9_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY9_GPIO, BOARD_CONFIG_HAL_KEY9_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY10_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY10_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY10_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY10_GPIO, BOARD_CONFIG_HAL_KEY10_MODE);
#endif
#if defined(BOARD_CONFIG_HAL_KEY11_GPIO)
    gpio_reset_pin(BOARD_CONFIG_HAL_KEY11_GPIO);
    gpio_set_direction(BOARD_CONFIG_HAL_KEY11_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOARD_CONFIG_HAL_KEY11_GPIO, BOARD_CONFIG_HAL_KEY11_MODE);
#endif
}

/*********************************************************************************************
* 名称：key_getState()
* 功能：KEY按键扫描
* 参数：无
* 返回：按键状态,每位表示一个按键状态，1触发，0没有触发
* 修改：
* 注释：
*********************************************************************************************/
unsigned int key_getState(void)
{
  unsigned int keyVal = 0;
#if defined(BOARD_CONFIG_HAL_KEY0_GPIO)
  if(BORAD_HAL_KEY0 == BOARD_CONFIG_HAL_KEY0_ACTIVATE)
    keyVal |= KEY0_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY1_GPIO)
  if(BORAD_HAL_KEY1 == BOARD_CONFIG_HAL_KEY1_ACTIVATE)
    keyVal |= KEY1_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY2_GPIO)
  if(BORAD_HAL_KEY2 == BOARD_CONFIG_HAL_KEY2_ACTIVATE)
    keyVal |= KEY2_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY3_GPIO)
  if(BORAD_HAL_KEY3 == BOARD_CONFIG_HAL_KEY3_ACTIVATE)
    keyVal |= KEY3_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY4_GPIO)
  if(BORAD_HAL_KEY4 == BOARD_CONFIG_HAL_KEY4_ACTIVATE)
    keyVal |= KEY4_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY5_GPIO)
  if(BORAD_HAL_KEY5 == BOARD_CONFIG_HAL_KEY5_ACTIVATE)
    keyVal |= KEY5_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY6_GPIO)
  if(BORAD_HAL_KEY6 == BOARD_CONFIG_HAL_KEY6_ACTIVATE)
    keyVal |= KEY6_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY7_GPIO)
  if(BORAD_HAL_KEY7 == BOARD_CONFIG_HAL_KEY7_ACTIVATE)
    keyVal |= KEY7_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY8_GPIO)
  if(BORAD_HAL_KEY8 == BOARD_CONFIG_HAL_KEY8_ACTIVATE)
    keyVal |= KEY8_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY9_GPIO)
  if(BORAD_HAL_KEY9 == BOARD_CONFIG_HAL_KEY9_ACTIVATE)
    keyVal |= KEY9_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY10_GPIO)
  if(BORAD_HAL_KEY10 == BOARD_CONFIG_HAL_KEY10_ACTIVATE)
    keyVal |= KEY10_PREESED;
#endif
#if defined(BOARD_CONFIG_HAL_KEY11_GPIO)
  if(BORAD_HAL_KEY11 == BOARD_CONFIG_HAL_KEY11_ACTIVATE)
    keyVal |= KEY11_PREESED;
#endif
  return keyVal;
}

/*********************************************************************************************
* 名称：u8 KEY_Scan(u8 mode)
* 功能：按键处理函数
* 参数：mode: 0,不支持连续按; 1,支持连续按;
* 返回：返回按键值
* 修改：
* 注释：注意此函数有响应优先级,KEY0>KEY1>KEY2>KEY3!!
* 测试代码：
*********************************************************************************************/
gpio_num_t KEY_Scan(uint8_t mode)
{
  static uint8_t key_up=1;                                           //按键按松开标志
  if(mode)key_up=1;                                             //支持连按
  if(key_up&&(  BORAD_HAL_KEY0==BOARD_CONFIG_HAL_KEY0_ACTIVATE || BORAD_HAL_KEY1==BOARD_CONFIG_HAL_KEY1_ACTIVATE || \
                BORAD_HAL_KEY2==BOARD_CONFIG_HAL_KEY2_ACTIVATE || BORAD_HAL_KEY3==BOARD_CONFIG_HAL_KEY3_ACTIVATE || \
                BORAD_HAL_KEY4==BOARD_CONFIG_HAL_KEY4_ACTIVATE || BORAD_HAL_KEY5==BOARD_CONFIG_HAL_KEY5_ACTIVATE || \
                BORAD_HAL_KEY6==BOARD_CONFIG_HAL_KEY6_ACTIVATE || BORAD_HAL_KEY7==BOARD_CONFIG_HAL_KEY7_ACTIVATE || \
                BORAD_HAL_KEY8==BOARD_CONFIG_HAL_KEY8_ACTIVATE || BORAD_HAL_KEY9==BOARD_CONFIG_HAL_KEY9_ACTIVATE || \
                BORAD_HAL_KEY10==BOARD_CONFIG_HAL_KEY10_ACTIVATE || BORAD_HAL_KEY11==BOARD_CONFIG_HAL_KEY11_ACTIVATE )
              )
  {
    delay_ms(10);                                               //去抖动 
    key_up=0;
    if(BORAD_HAL_KEY0==BOARD_CONFIG_HAL_KEY0_ACTIVATE)return KEY0_PREESED;
    else if(BORAD_HAL_KEY1==BOARD_CONFIG_HAL_KEY1_ACTIVATE)return KEY1_PREESED;
    else if(BORAD_HAL_KEY2==BOARD_CONFIG_HAL_KEY2_ACTIVATE)return KEY2_PREESED;
    else if(BORAD_HAL_KEY3==BOARD_CONFIG_HAL_KEY3_ACTIVATE)return KEY3_PREESED;
    else if(BORAD_HAL_KEY4==BOARD_CONFIG_HAL_KEY4_ACTIVATE)return KEY4_PREESED;
    else if(BORAD_HAL_KEY5==BOARD_CONFIG_HAL_KEY5_ACTIVATE)return KEY5_PREESED;
    else if(BORAD_HAL_KEY6==BOARD_CONFIG_HAL_KEY6_ACTIVATE)return KEY6_PREESED;
    else if(BORAD_HAL_KEY7==BOARD_CONFIG_HAL_KEY7_ACTIVATE)return KEY7_PREESED;
    else if(BORAD_HAL_KEY8==BOARD_CONFIG_HAL_KEY8_ACTIVATE)return KEY8_PREESED;
    else if(BORAD_HAL_KEY9==BOARD_CONFIG_HAL_KEY9_ACTIVATE)return KEY9_PREESED;
    else if(BORAD_HAL_KEY10==BOARD_CONFIG_HAL_KEY10_ACTIVATE)return KEY10_PREESED;
    else if(BORAD_HAL_KEY11==BOARD_CONFIG_HAL_KEY11_ACTIVATE)return KEY11_PREESED;
  }else if( BORAD_HAL_KEY0==(!BOARD_CONFIG_HAL_KEY0_ACTIVATE) && BORAD_HAL_KEY1==(!BOARD_CONFIG_HAL_KEY1_ACTIVATE) && \
            BORAD_HAL_KEY2==(!BOARD_CONFIG_HAL_KEY2_ACTIVATE) && BORAD_HAL_KEY3==(!BOARD_CONFIG_HAL_KEY3_ACTIVATE) && \
            BORAD_HAL_KEY4==(!BOARD_CONFIG_HAL_KEY4_ACTIVATE) && BORAD_HAL_KEY5==(!BOARD_CONFIG_HAL_KEY5_ACTIVATE) && \
            BORAD_HAL_KEY6==(!BOARD_CONFIG_HAL_KEY6_ACTIVATE) && BORAD_HAL_KEY7==(!BOARD_CONFIG_HAL_KEY7_ACTIVATE) && \
            BORAD_HAL_KEY8==(!BOARD_CONFIG_HAL_KEY8_ACTIVATE) && BORAD_HAL_KEY9==(!BOARD_CONFIG_HAL_KEY9_ACTIVATE) && \
            BORAD_HAL_KEY10==(!BOARD_CONFIG_HAL_KEY10_ACTIVATE) && BORAD_HAL_KEY11==(!BOARD_CONFIG_HAL_KEY11_ACTIVATE)
          ) key_up=1;
  return GPIO_NUM_NC;                                                     // 无按键按下
}

// 测试获取按键状态
void test_key_getState_fun(void)
{
    unsigned int key = key_getState();
    print_binary(key,32);
    vTaskDelay(pdMS_TO_TICKS(100));
}

// 测试键值扫描
void test_KEY_Scan_fun(void)
{
    gpio_num_t key = KEY_Scan(0);
    if(key!=GPIO_NUM_NC){
        printf("key:%d\r\n",key);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}

/*********************************************************************************************
* 名称：key_thread_entry()
* 功能：KEY线程入口函数
* 参数：*parameter -> 入口参数
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
// static void key_thread_entry(void *parameter)
// {
//   (void)parameter;
//   unsigned char keyVal = 0;
//   key_pin_init();
//   while(1)
//   {
//     // if(app_event==RT_NULL) goto wait;
// #ifdef EVENT_DEBUG
//     int _rand = rand() % 7 ;
//     rt_event_send(app_event, 1<<_rand);
// #endif
//     if(keyVal){
//     //   if(key_getState() == 0x00){
//     //     rt_event_send(app_event, keyVal);
//     //     keyVal = 0;
//     //   }
//     }
//     else
//       keyVal = key_getState();
// wait: rt_thread_mdelay(100);
//   }
// }
