#ifndef __DRV_INPUT_H__
#define __DRV_INPUT_H__
#include "board.h"
#include "apl_atomic.h"

/*********************************************************************************************
* 配置
*********************************************************************************************/
// #define BOARD_CONFIG_INPUT_OBJS_NUM             3                           // 申请多少个 in 对象
// #define BOARD_CONFIG_INPUT_TOUT_EVT_NUM         2                           // 使用超时事件的个数
// #define BOARD_CONFIG_IN_GET_MS                  aos_now_ms                  // 获取ms时间
// // #define BOARD_CONFIG_INPUT_ACT_INDICATION      gpio_fast_write(LEDLight,0) // 输入激活指示
// // #define BOARD_CONFIG_INPUT_INACT_INDICATION        gpio_fast_write(LEDLight,1) // 输入非激活指示
// #define BOARD_CONFIG_INPUT_ACT_INDICATION 
// #define BOARD_CONFIG_INPUT_INACT_INDICATION 

/*********************************************************************************************
* 定义
*********************************************************************************************/
#define INVALID_TIMEOUT_PARAMETER           ((uint32_t)0xffffffff)

/*********************************************************************************************
* 类型
*********************************************************************************************/
/* 输入对象回调函数 */
typedef int (*active_tout_cb_t)(uint32_t index,uint64_t active_time,uint64_t elapsed_time,void* user_data);
typedef int (*inact_cb_t)(uint32_t index,uint64_t active_time,uint64_t inact_time,void* user_data);
typedef int (*read_level)(uint32_t index);
/* 组回调函数 */
typedef int (*gp_active_tout_cb_t)(atomic_t* in_mask,atomic_t* Tout_mask,atomic_t* Tout_maskbk,atomic_t* inact_mask,void* user_data);
typedef int (*gp_ins_scan_cb_t)(void* user_data);

/**********  输入对象 **********/
typedef struct 
{
    /* 激活超时时间和对应超时函数,单位ms，0 则表示无效 */
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 1
    uint32_t timeout0;
    active_tout_cb_t active_tout_cb0;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
    uint32_t timeout1;
    active_tout_cb_t active_tout_cb1;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
    uint32_t timeout2;
    active_tout_cb_t active_tout_cb2;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
    uint32_t timeout3;
    active_tout_cb_t active_tout_cb3;
#endif
}input_Tout_cb_t;

typedef struct 
{
    /* 状态位 */
    atomic_t Status;

    uint64_t active_time;        // 激活记录时间
    uint64_t inact_time;         // 非激活记录时间

    /* 事件组 */
    // atomic_t event;
    // atomic_t sec_event;

    /* 读状态 */
    read_level read;

    /* 超时回调 */
    input_Tout_cb_t TimOut_cb;

    /* 非激活回调函数 */
    inact_cb_t inact_cb;
}input_obj_t;

#if BOARD_CONFIG_EN_INPUT_GP == 1
/*********** 输入组对象 ***********/
typedef struct 
{
    /* 激活超时时间和对应超时函数,单位ms，0 则表示无效 */
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 1
    gp_active_tout_cb_t GPactive_tout_cb0;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
    gp_active_tout_cb_t GPactive_tout_cb1;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
    gp_active_tout_cb_t GPactive_tout_cb2;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
    gp_active_tout_cb_t GPactive_tout_cb3;
#endif
}input_gp_cb_t;

typedef struct 
{
    /* 超时回调 */
    input_gp_cb_t gp_TimOut_cb;
    /* 成员键值扫描 */
    gp_ins_scan_cb_t scan;

    /* 组成员实时状态 */
    atomic_t in_mask;
    /* 组成员非激活标记 */
    atomic_t inact_mask;
    /* 所有的输入公共MASK */
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 1
    atomic_t in_tout0_mask;        // 所有输入的超时事件0掩码,通知一次便清除，一次触发一次置位
    atomic_t in_tout0_maskbk;      // 一次触发一直置位,应用层控制复位
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
    atomic_t in_tout1_mask;
    atomic_t in_tout1_maskbk;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
    atomic_t in_tout2_mask;
    atomic_t in_tout2_maskbk;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
    atomic_t in_tout3_mask;
    atomic_t in_tout3_maskbk;
#endif
    /* 成员指针 */
    input_obj_t* objs;
    uint32_t obj_num;
}input_gp_t;
#endif  /* BOARD_CONFIG_EN_INPUT_GP */

/*********************************************************************************************
* 外部变量申明
*********************************************************************************************/
// extern input_obj_t g_in_objs[BOARD_CONFIG_INPUT_OBJS_NUM];

/*********************************************************************************************
* 函数
*********************************************************************************************/
void Input_Scan(void);
int Input_Obj_Init(int index, input_Tout_cb_t* TimOut_cb, inact_cb_t inact_cb, read_level read);
#if BOARD_CONFIG_EN_INPUT_GP == 1
    int Input_Group_Init(input_gp_cb_t* timout_cb, gp_ins_scan_cb_t scan);
#endif
#endif
