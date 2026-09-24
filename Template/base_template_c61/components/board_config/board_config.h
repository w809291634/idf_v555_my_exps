#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

/*********************************************************************************************
* 引脚配置
*********************************************************************************************/
/* LED config  */
// #define BOARD_CONFIG_HAL_LED0_GPIO        4
// #define BOARD_CONFIG_HAL_LED0_ACTIVATE    0
// #define BOARD_CONFIG_HAL_LED1_GPIO        5
// #define BOARD_CONFIG_HAL_LED1_ACTIVATE    0

/* KEY HAL */
//#define BOARD_CONFIG_HAL_KEY0_PORT        E
//#define BOARD_CONFIG_HAL_KEY0_GPIO        4
//#define BOARD_CONFIG_HAL_KEY0_MODE        GPIO_PULLUP // GPIO_PULLDOWN
//#define BOARD_CONFIG_HAL_KEY0_ACTIVATE    0

//#define BOARD_CONFIG_HAL_KEY1_PORT        E
//#define BOARD_CONFIG_HAL_KEY1_GPIO        3
//#define BOARD_CONFIG_HAL_KEY1_MODE        GPIO_PULLUP 
//#define BOARD_CONFIG_HAL_KEY1_ACTIVATE    0

//#define BOARD_CONFIG_HAL_KEY2_PORT        A
//#define BOARD_CONFIG_HAL_KEY2_GPIO        0
//#define BOARD_CONFIG_HAL_KEY2_MODE        GPIO_PULLDOWN
//#define BOARD_CONFIG_HAL_KEY2_ACTIVATE    1

/*********************************************************************************************
* 任务配置
*********************************************************************************************/
// console
#define BOARD_CONFIG_CONSOLE_TASK_STACK_SIZE           4096
#define BOARD_CONFIG_CONSOLE_TASK_PRIOR                10
#define BOARD_CONFIG_CONSOLE_TASK_CPU                  0



#endif
