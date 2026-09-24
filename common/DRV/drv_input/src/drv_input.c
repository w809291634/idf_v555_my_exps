#include "drv_input.h"

#define DBG_TAG           "drvin"
//#define DBG_LVL           DBG_INFO
// #define DBG_LVL           DBG_LOG
#define DBG_LVL           DBG_NODBG
#include <mydbg.h>          // must after of DBG_LVL, DBG_TAG or other options

#if BOARD_CONFIG_INPUT_EN == 1

/*********************************************************************************************
* 配置
*********************************************************************************************/
/* 所有的输入对象 */
static input_obj_t g_in_objs[BOARD_CONFIG_INPUT_OBJS_NUM]={0,};
#if BOARD_CONFIG_EN_INPUT_GP == 1
    static input_gp_t g_in_gp_obj={0,};
#endif

// 输入对象状态位
#define INPUT_ACTIVATION                           0   // 输入激活
#define INPUT_DELAY_RESET                          1   // 延迟复位

#define INPUT_ACTIVE_TIMEOUT0_EVT                   8
#define INPUT_ACTIVE_TIMEOUT1_EVT                   9
#define INPUT_ACTIVE_TIMEOUT2_EVT                   10
#define INPUT_ACTIVE_TIMEOUT3_EVT                   11

#define INPUT_ACTIVE_TIMEOUT0_BKEVT                 12
#define INPUT_ACTIVE_TIMEOUT1_BKEVT                 13
#define INPUT_ACTIVE_TIMEOUT2_BKEVT                 14
#define INPUT_ACTIVE_TIMEOUT3_BKEVT                 15
/*********************************************************************************************
* 定义
*********************************************************************************************/
// 所有的超时事件
#define INPUT_TIMEOUT_EVT_MASK \
        ((1 << INPUT_ACTIVE_TIMEOUT0_EVT) | \
        (1 << INPUT_ACTIVE_TIMEOUT1_EVT) | \
        (1 << INPUT_ACTIVE_TIMEOUT2_EVT) | \
        (1 << INPUT_ACTIVE_TIMEOUT3_EVT))

// 所有的超时事件
#define INPUT_TIMEOUT_BKEVT_MASK \
        ((1 << INPUT_ACTIVE_TIMEOUT0_BKEVT) | \
        (1 << INPUT_ACTIVE_TIMEOUT1_BKEVT) | \
        (1 << INPUT_ACTIVE_TIMEOUT2_BKEVT) | \
        (1 << INPUT_ACTIVE_TIMEOUT3_BKEVT))

/*********************************************************************************************
* 变量
*********************************************************************************************/

/*********************************************************************************************
* 函数
*********************************************************************************************/
#if BOARD_CONFIG_EN_INPUT_GP == 1
// 输入扫描
void Input_Scan(void)
{
    int ret = 0;

    /* 获取所有输入状态 */
    if(g_in_gp_obj.scan) g_in_gp_obj.scan(&g_in_gp_obj);
    /* 触发对应事件 */
    for(int i = 0;i<BOARD_CONFIG_INPUT_OBJS_NUM; i++){
        if(g_in_objs[i].read == NULL) continue;

        if(g_in_objs[i].read(i)){
            BOARD_CONFIG_INPUT_ACT_INDICATION;
            /* 输入触发 */
            ret = atomic_test_and_set_bit(&g_in_objs[i].Status,INPUT_ACTIVATION);
            if(ret == 0){
                log_d("in%u active",i);
                g_in_objs[i].active_time = BOARD_CONFIG_IN_GET_MS();
            }

            uint64_t elapsed_time = BOARD_CONFIG_IN_GET_MS() - g_in_objs[i].active_time;

            /* 组标记 */
            atomic_set_bit(&g_in_gp_obj.in_mask,i);

#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 1
            /* 触发激活超时事件0,常用于输入去抖 */
            if(g_in_objs[i].TimOut_cb.timeout0 && elapsed_time >= g_in_objs[i].TimOut_cb.timeout0){
                // 输入触发
                ret = atomic_test_and_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT0_BKEVT);
                if(ret == 0){
                    // 触发事件
                    atomic_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT0_EVT);
                    // 触发回调函数
                    if(g_in_objs[i].TimOut_cb.active_tout_cb0)
                        g_in_objs[i].TimOut_cb.active_tout_cb0(i,g_in_objs[i].active_time,elapsed_time,&g_in_objs);
                    // 组标记
                    atomic_set_bit(&g_in_gp_obj.in_tout0_mask,i);
                }
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
                // 仅仅在 timeout0 - timeout1 一直置位，否则导致T0 组的单击失效
                if(g_in_objs[i].TimOut_cb.timeout1 && elapsed_time < g_in_objs[i].TimOut_cb.timeout1){
                    atomic_set_bit(&g_in_gp_obj.in_tout0_maskbk,i);
                }
#else
                atomic_set_bit(&g_in_gp_obj.in_tout0_maskbk,i);
#endif
            }else continue;
#endif
            /* 输入去抖后清除组 非激活标记 */
            atomic_clear_bit(&g_in_gp_obj.inact_mask,i);
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
            /* 触发激活超时事件1 */
            if(g_in_objs[i].TimOut_cb.timeout1 && elapsed_time >= g_in_objs[i].TimOut_cb.timeout1){
                // 输入触发
                ret = atomic_test_and_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT1_BKEVT);
                if(ret == 0){
                    // 触发事件
                    atomic_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT1_EVT);
                    // 触发回调函数
                    if(g_in_objs[i].TimOut_cb.active_tout_cb1)
                        g_in_objs[i].TimOut_cb.active_tout_cb1(i,g_in_objs[i].active_time,elapsed_time,&g_in_objs);
                    // 全局标记
                    atomic_set_bit(&g_in_gp_obj.in_tout1_mask,i);
                }
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
                // 仅仅在 timeout1 - timeout2 一直置位，否则导致T1 组的单击失效
                if(g_in_objs[i].TimOut_cb.timeout2 && elapsed_time < g_in_objs[i].TimOut_cb.timeout2){
                    atomic_set_bit(&g_in_gp_obj.in_tout1_maskbk,i);
                }
#else
                atomic_set_bit(&g_in_gp_obj.in_tout1_maskbk,i);
#endif
            }else continue;
#endif

#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
            /* 触发激活超时事件2 */
            if(g_in_objs[i].TimOut_cb.timeout2 && elapsed_time >= g_in_objs[i].TimOut_cb.timeout2){
                // 输入触发
                ret = atomic_test_and_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT2_BKEVT);
                if(ret == 0){
                    // 触发事件
                    atomic_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT2_EVT);
                    // 触发回调函数
                    if(g_in_objs[i].TimOut_cb.active_tout_cb2)
                        g_in_objs[i].TimOut_cb.active_tout_cb2(i,g_in_objs[i].active_time,elapsed_time,&g_in_objs);
                    // 全局标记
                    atomic_set_bit(&g_in_gp_obj.in_tout2_mask,i);
                }
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
                // 仅仅在 timeout2 - timeout3 一直置位，否则导致T2 组的单击失效
                if(g_in_objs[i].TimOut_cb.timeout3 && elapsed_time < g_in_objs[i].TimOut_cb.timeout3){
                    atomic_set_bit(&g_in_gp_obj.in_tout2_maskbk,i);
                }
#else
                atomic_set_bit(&g_in_gp_obj.in_tout2_maskbk,i);
#endif
            }else continue;
#endif

#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
            /* 触发激活超时事件3 */
            if(g_in_objs[i].TimOut_cb.timeout3 && elapsed_time >= g_in_objs[i].TimOut_cb.timeout3){
                // 输入触发
                ret = atomic_test_and_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT3_BKEVT);
                if(ret == 0){
                    // 触发事件
                    atomic_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT3_EVT);
                    // 触发回调函数
                    if(g_in_objs[i].TimOut_cb.active_tout_cb3)
                        g_in_objs[i].TimOut_cb.active_tout_cb3(i,g_in_objs[i].active_time,elapsed_time,&g_in_objs);
                    // 全局标记
                    atomic_set_bit(&g_in_gp_obj.in_tout3_mask,i);
                }
                atomic_set_bit(&g_in_gp_obj.in_tout3_maskbk,i);
            }else continue;
#endif
        }else{
            /* 清除组标记 */
            atomic_clear_bit(&g_in_gp_obj.in_mask,i);
#if 0
            // 延迟一个周期复位
            ret = atomic_test_and_clear_bit(&g_in_objs[i].Status,INPUT_DELAY_RESET);
            if(ret == 1){
                // 清理组标记
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 1
                atomic_clear_bit(&g_in_gp_obj.in_tout0_maskbk,i);
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
                atomic_clear_bit(&g_in_gp_obj.in_tout1_maskbk,i);
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
                atomic_clear_bit(&g_in_gp_obj.in_tout2_maskbk,i);
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
                atomic_clear_bit(&g_in_gp_obj.in_tout3_maskbk,i);
#endif
            }
#endif /* 0 */
            // 输入非激活
            ret = atomic_test_and_clear_bit(&g_in_objs[i].Status,INPUT_ACTIVATION);
            if(ret == 1){
                log_d("in%u inact",i);
                if( atomic_test_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT0_BKEVT) || 
                    atomic_test_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT1_BKEVT) || 
                    atomic_test_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT2_BKEVT) || 
                    atomic_test_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT3_BKEVT)
                ){
                    g_in_objs[i].inact_time = BOARD_CONFIG_IN_GET_MS();
                    // 触发回调函数
                    if(g_in_objs[i].inact_cb)
                        g_in_objs[i].inact_cb(i,g_in_objs[i].active_time,g_in_objs[i].inact_time,&g_in_objs);
                    // 设置组 非激活标记
                    atomic_set_bit(&g_in_gp_obj.inact_mask,i);
                }
                BOARD_CONFIG_INPUT_INACT_INDICATION;

                // 清理所有激活超时事件
                atomic_and(&g_in_objs[i].Status, ~INPUT_TIMEOUT_EVT_MASK);
                atomic_and(&g_in_objs[i].Status, ~INPUT_TIMEOUT_BKEVT_MASK);
                // 清理组标记
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 1
                atomic_clear_bit(&g_in_gp_obj.in_tout0_mask,i);
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
                atomic_clear_bit(&g_in_gp_obj.in_tout1_mask,i);
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
                atomic_clear_bit(&g_in_gp_obj.in_tout2_mask,i);
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
                atomic_clear_bit(&g_in_gp_obj.in_tout3_mask,i);
#endif
                // 触发延迟复位
                atomic_set_bit(&g_in_objs[i].Status,INPUT_DELAY_RESET);
            }
        }
    }
    /* 输入组事件回调 */
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
    if(g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb3)
        g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb3( &g_in_gp_obj.in_mask,
                                                    &g_in_gp_obj.in_tout3_mask,
                                                    &g_in_gp_obj.in_tout3_maskbk,
                                                    &g_in_gp_obj.inact_mask,
                                                    &g_in_gp_obj);
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
    if(g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb2)
        g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb2( &g_in_gp_obj.in_mask,
                                                    &g_in_gp_obj.in_tout2_mask,
                                                    &g_in_gp_obj.in_tout2_maskbk,
                                                    &g_in_gp_obj.inact_mask,
                                                    &g_in_gp_obj);
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
    if(g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb1)
        g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb1( &g_in_gp_obj.in_mask,
                                                    &g_in_gp_obj.in_tout1_mask,
                                                    &g_in_gp_obj.in_tout1_maskbk,
                                                    &g_in_gp_obj.inact_mask,
                                                    &g_in_gp_obj);
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 1
    if(g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb0)
        g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb0( &g_in_gp_obj.in_mask,
                                                    &g_in_gp_obj.in_tout0_mask,
                                                    &g_in_gp_obj.in_tout0_maskbk,
                                                    &g_in_gp_obj.inact_mask,
                                                    &g_in_gp_obj);
#endif
}

#else
// 输入扫描
void Input_Scan(void)
{
    int ret = 0;

    /* 获取所有输入状态 */
    BOARD_CONFIG_ALL_SCAN_INPUT;
    /* 触发对应事件 */
    for(int i = 0;i<BOARD_CONFIG_INPUT_OBJS_NUM; i++){
        if(g_in_objs[i].read == NULL) continue;

        if(g_in_objs[i].read(i)){
            BOARD_CONFIG_INPUT_ACT_INDICATION;
            /* 输入触发 */
            ret = atomic_test_and_set_bit(&g_in_objs[i].Status,INPUT_ACTIVATION);
            if(ret == 0){
                log_d("in%u active",i);
                g_in_objs[i].active_time = BOARD_CONFIG_IN_GET_MS();
            }

            uint64_t elapsed_time = BOARD_CONFIG_IN_GET_MS() - g_in_objs[i].active_time;

#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 1
            /* 触发激活超时事件0,常用于输入去抖 */
            if(g_in_objs[i].TimOut_cb.timeout0 && elapsed_time >= g_in_objs[i].TimOut_cb.timeout0){
                // 输入触发
                ret = atomic_test_and_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT0_BKEVT);
                if(ret == 0){
                    // 触发事件
                    atomic_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT0_EVT);
                    // 触发回调函数
                    if(g_in_objs[i].TimOut_cb.active_tout_cb0)
                        g_in_objs[i].TimOut_cb.active_tout_cb0(i,g_in_objs[i].active_time,elapsed_time,&g_in_objs);
                }
            }else continue;
#endif

#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
            /* 触发激活超时事件1 */
            if(g_in_objs[i].TimOut_cb.timeout1 && elapsed_time >= g_in_objs[i].TimOut_cb.timeout1){
                // 输入触发
                ret = atomic_test_and_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT1_BKEVT);
                if(ret == 0){
                    // 触发事件
                    atomic_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT1_EVT);
                    // 触发回调函数
                    if(g_in_objs[i].TimOut_cb.active_tout_cb1)
                        g_in_objs[i].TimOut_cb.active_tout_cb1(i,g_in_objs[i].active_time,elapsed_time,&g_in_objs);
                }
            }else continue;
#endif

#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
            /* 触发激活超时事件2 */
            if(g_in_objs[i].TimOut_cb.timeout2 && elapsed_time >= g_in_objs[i].TimOut_cb.timeout2){
                // 输入触发
                ret = atomic_test_and_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT2_BKEVT);
                if(ret == 0){
                    // 触发事件
                    atomic_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT2_EVT);
                    // 触发回调函数
                    if(g_in_objs[i].TimOut_cb.active_tout_cb2)
                        g_in_objs[i].TimOut_cb.active_tout_cb2(i,g_in_objs[i].active_time,elapsed_time,&g_in_objs);
                }
            }else continue;
#endif

#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
            /* 触发激活超时事件3 */
            if(g_in_objs[i].TimOut_cb.timeout3 && elapsed_time >= g_in_objs[i].TimOut_cb.timeout3){
                // 输入触发
                ret = atomic_test_and_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT3_BKEVT);
                if(ret == 0){
                    // 触发事件
                    atomic_set_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT3_EVT);
                    // 触发回调函数
                    if(g_in_objs[i].TimOut_cb.active_tout_cb3)
                        g_in_objs[i].TimOut_cb.active_tout_cb3(i,g_in_objs[i].active_time,elapsed_time,&g_in_objs);
                }
            }else continue;
#endif
        }else{
            // 输入非激活
            ret = atomic_test_and_clear_bit(&g_in_objs[i].Status,INPUT_ACTIVATION);
            if(ret == 1){
                log_d("in%u inact",i);
                if( atomic_test_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT0_BKEVT) || 
                    atomic_test_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT1_BKEVT) || 
                    atomic_test_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT2_BKEVT) || 
                    atomic_test_bit(&g_in_objs[i].Status,INPUT_ACTIVE_TIMEOUT3_BKEVT)
                ){
                    g_in_objs[i].inact_time = BOARD_CONFIG_IN_GET_MS();
                    // 触发回调函数
                    if(g_in_objs[i].inact_cb)
                        g_in_objs[i].inact_cb(i,g_in_objs[i].active_time,g_in_objs[i].inact_time,&g_in_objs);
                }
                BOARD_CONFIG_INPUT_INACT_INDICATION;

                // 清理所有激活超时事件
                atomic_and(&g_in_objs[i].Status, ~INPUT_TIMEOUT_EVT_MASK);
                atomic_and(&g_in_objs[i].Status, ~INPUT_TIMEOUT_BKEVT_MASK);

                // 触发延迟复位
                atomic_set_bit(&g_in_objs[i].Status,INPUT_DELAY_RESET);
            }
        }
    }
}

#endif

// 输入对象初始化
int Input_Obj_Init(int index, input_Tout_cb_t* timout_cb, inact_cb_t inact_cb, read_level read)
{
    _ASSERT(index < BOARD_CONFIG_INPUT_OBJS_NUM);
    
    input_obj_t* in = &g_in_objs[index];

    // 指针置空
    in->inact_cb = NULL;
    in->read = NULL;
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 1
    in->TimOut_cb.timeout0 = 0;
    in->TimOut_cb.active_tout_cb0 = NULL;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
    in->TimOut_cb.timeout1 = 0;
    in->TimOut_cb.active_tout_cb1 = NULL;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
    in->TimOut_cb.timeout2 = 0;
    in->TimOut_cb.active_tout_cb2 = NULL;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
    in->TimOut_cb.timeout3 = 0;
    in->TimOut_cb.active_tout_cb3 = NULL;
#endif
    // 复制超时回调配置
    if(timout_cb)
        memcpy(&in->TimOut_cb, timout_cb, sizeof(input_Tout_cb_t));
    // 非激活回调
    if(inact_cb) 
        in->inact_cb = inact_cb;
    // 读回调
    if(read)
        in->read = read;

    // 初始化其他必要字段
    in->Status = ATOMIC_INIT(0);
    // in->event = ATOMIC_INIT(0);
    // in->sec_event = ATOMIC_INIT(0);

    // 时间记忆
    in->active_time = 0;            // 激活时间清零
    in->inact_time = 0;             // 非激活时间清零
    return 0;
}

#if BOARD_CONFIG_EN_INPUT_GP == 1
// 输入组初始化
int Input_Group_Init(input_gp_cb_t* timout_cb,gp_ins_scan_cb_t scan)
{
    // 成员变量初始化
    g_in_gp_obj.objs = g_in_objs;
    g_in_gp_obj.obj_num = BOARD_CONFIG_INPUT_OBJS_NUM;
    g_in_gp_obj.in_mask = ATOMIC_INIT(0);
    g_in_gp_obj.inact_mask = ATOMIC_INIT(0);
    // 输入超时函数注册
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 1
    g_in_gp_obj.in_tout0_mask = ATOMIC_INIT(0);
    g_in_gp_obj.in_tout0_maskbk = ATOMIC_INIT(0);
    g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb0 = NULL;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 2
    g_in_gp_obj.in_tout1_mask = ATOMIC_INIT(0);
    g_in_gp_obj.in_tout1_maskbk = ATOMIC_INIT(0);
    g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb1 = NULL;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 3
    g_in_gp_obj.in_tout2_mask = ATOMIC_INIT(0);
    g_in_gp_obj.in_tout2_maskbk = ATOMIC_INIT(0);
    g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb2 = NULL;
#endif
#if BOARD_CONFIG_INPUT_TOUT_EVT_NUM >= 4
    g_in_gp_obj.in_tout3_mask = ATOMIC_INIT(0);
    g_in_gp_obj.in_tout3_maskbk = ATOMIC_INIT(0);
    g_in_gp_obj.gp_TimOut_cb.GPactive_tout_cb3 = NULL;
#endif
    // 复制超时回调配置
    if(timout_cb)
        memcpy(&g_in_gp_obj.gp_TimOut_cb, timout_cb, sizeof(input_gp_cb_t));
    // 输入扫描程序
    if(scan)
        g_in_gp_obj.scan = scan;
    else g_in_gp_obj.scan = NULL;

    return 0;
}
#endif

#endif // BOARD_CONFIG_INPUT_EN