/*****************************************************************************
 * Copyright (C) 2026 看门狗模块贡献者。
 *
 * 保留所有权利。
 *
 * @file service_watchdog_monitor.c
 *
 * @par dependencies
 * - service_watchdog_monitor.h
 * - watchdog_config.h
 * - watchdog_adapter_port.h
 * - osal.h
 * - string.h
 *
 * @author YA
 *
 * @brief 提供任务超时监控、启动失败计数和安全模式标志。
 *
 * 处理流程：
 * 由上层初始化并调用接口，具体操作见各函数说明。
 *
 * @version V1.0
 *
 * @note 1 个制表符等于 4 个空格；注释采用 UTF-8 中文。
 * @note 移植自 ec_s100_watch_V2.5_T3605 工程的看门狗监控服务。
 * @note 时间戳差值与毫秒超时直接比较，OSAL 时基须与此一致。
 * @note 获取监控锁失败时，本轮仍按健康状态刷新硬件看门狗。
 *
 ****************************************************************************/

//****************************** Includes begin ******************************//
#include "service_watchdog_monitor.h"
#include "watchdog_config.h"
#include "watchdog_adapter_port.h"
#include "osal.h"
#include <string.h>
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
#ifndef WDOG_LOG
#define WDOG_LOG(fmt, ...) ((void)0)
#endif
//******************************* Defines end ********************************//

//****************************** Structs begin *******************************//
typedef struct
{
    osal_task_handle_t task_handle;
    uint32_t last_feed_time;
    uint32_t timeout_ms;
    char task_name[WATCHDOG_TASK_NAME_LEN];
    bool is_active;
} watchdog_entry_t;
//******************************* Structs end ********************************//

//************************** Static Variables begin **************************//
static watchdog_entry_t s_watchdogEntries[MAX_WATCHDOG_TASKS];
static osal_mutex_handle_t s_watchdogMutex = NULL;
static volatile bool s_isWatchdogPaused = false;
static uint32_t s_bootFailCount = 0U;
static bool s_isSafeMode = false;
static bool s_isWatchdogInitialized = false;
//*************************** Static Variables end ***************************//

//************************ Function Prototypes begin *************************//
static void watchdog_entry_reset(watchdog_entry_t *p_entry);
static void watchdog_entries_init(void);
//************************* Function Prototypes end **************************//

//************************ Function Definitions begin ************************//

/**
 * @brief 重置监控表项。
 *
 * 步骤：
 *  1. 检查指针，清空任务信息并恢复默认超时。
 *
 * @param[in,out] p_entry : 待重置的监控表项指针。
 *
 * @return 无；空指针时直接返回。
 *
 */
static void watchdog_entry_reset(watchdog_entry_t *p_entry)
{
    //-------------------------参数检查 begin-------------------------//
    if (p_entry == NULL)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    p_entry->task_handle = NULL;
    p_entry->last_feed_time = 0U;
    p_entry->timeout_ms = WATCHDOG_TIMEOUT_MS;
    p_entry->task_name[0] = '\0';
    p_entry->is_active = false;
}

/**
 * @brief 初始化软件监控表。
 *
 * 步骤：
 *  1. 遍历监控表，逐项重置。
 *
 * @param 无。
 *
 * @return 无。
 *
 */
static void watchdog_entries_init(void)
{
    uint32_t index;

    for (index = 0U; index < MAX_WATCHDOG_TASKS; index++)
    {
        watchdog_entry_reset(&s_watchdogEntries[index]);
    }
}

/**
 * @brief 初始化看门狗服务。
 *
 * 步骤：
 *  1. 检查初始化标志，初始化硬件并创建互斥锁。
 *  2. 重置监控表，读取计数，若为 IWDG 复位则递增并保存。
 *  3. 计数达到阈值时置安全模式标志，最后标记初始化完成。
 *
 * @param 无。
 *
 * @return 无；创建互斥锁失败时提前返回。
 *
 */
void watchdog_service_init(void)
{
    if (s_isWatchdogInitialized)
    {
        return;
    }

    watchdog_port_init();

    if ((s_watchdogMutex == NULL) &&
        (osal_mutex_create(&s_watchdogMutex) != OSAL_SUCCESS))
    {
        WDOG_LOG("Watchdog mutex create failed!\r\n");
        return;
    }

    watchdog_entries_init();

    s_bootFailCount = watchdog_port_boot_fail_read();
    if (watchdog_port_is_wdg_reset())
    {
        s_bootFailCount++;
        watchdog_port_boot_fail_write(s_bootFailCount);
        WDOG_LOG("System reset by Watchdog! Fail count: %lu\r\n",
                 (unsigned long)s_bootFailCount);
    }

    if (s_bootFailCount >= BOOT_FAIL_THRESHOLD)
    {
        s_isSafeMode = true;
        WDOG_LOG("Entering SAFE MODE due to repeated crashes.\r\n");
    }

    s_isWatchdogInitialized = true;
}

/**
 * @brief 注册任务到首个空闲监控表项。
 *
 * 步骤：
 *  1. 检查参数并获取互斥锁。
 *  2. 记录任务、超时、当前时刻和名称，释放互斥锁。
 *
 * @param[in] handle : 待监控的任务句柄。
 * @param[in] timeout_ms : 允许的喂狗间隔，单位毫秒。
 * @param[in] name : 任务名称，超长部分截断并补终止符。
 *
 * @return 无；参数无效、获取锁失败或表满时不注册。
 *
 */
void watchdog_register(osal_task_handle_t handle, uint32_t timeout_ms, const char *name)
{
    uint32_t index;

    //-------------------------参数检查 begin-------------------------//
    if ((s_watchdogMutex == NULL) || (handle == NULL) || (name == NULL))
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    if (osal_mutex_take(s_watchdogMutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS)
    {
        return;
    }

    for (index = 0U; index < MAX_WATCHDOG_TASKS; index++)
    {
        if (!s_watchdogEntries[index].is_active)
        {
            s_watchdogEntries[index].task_handle = handle;
            s_watchdogEntries[index].timeout_ms = timeout_ms;
            s_watchdogEntries[index].last_feed_time = osal_task_get_tick_count();
            (void)strncpy(s_watchdogEntries[index].task_name, name,
                          sizeof(s_watchdogEntries[index].task_name) - 1U);
            s_watchdogEntries[index].task_name[sizeof(s_watchdogEntries[index].task_name) - 1U] = '\0';
            s_watchdogEntries[index].is_active = true;
            break;
        }
    }

    osal_mutex_give(s_watchdogMutex);
}

/**
 * @brief 注销首个匹配的活动任务表项。
 *
 * 步骤：
 *  1. 检查参数并获取互斥锁。
 *  2. 重置匹配表项，释放互斥锁。
 *
 * @param[in] handle : 待注销任务的句柄。
 *
 * @return 无；参数无效或获取锁失败时提前返回。
 *
 */
void watchdog_unregister(osal_task_handle_t handle)
{
    uint32_t index;

    //-------------------------参数检查 begin-------------------------//
    if ((s_watchdogMutex == NULL) || (handle == NULL))
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    if (osal_mutex_take(s_watchdogMutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS)
    {
        return;
    }

    for (index = 0U; index < MAX_WATCHDOG_TASKS; index++)
    {
        if (s_watchdogEntries[index].is_active &&
            (s_watchdogEntries[index].task_handle == handle))
        {
            watchdog_entry_reset(&s_watchdogEntries[index]);
            break;
        }
    }

    osal_mutex_give(s_watchdogMutex);
}

/**
 * @brief 更新首个匹配任务表项的喂狗时刻。
 *
 * 步骤：
 *  1. 检查暂停状态、互斥锁和句柄，再获取互斥锁。
 *  2. 更新时间戳，释放互斥锁。
 *
 * @param[in] handle : 已注册任务的句柄。
 *
 * @return 无；暂停、参数无效或获取锁失败时提前返回。
 *
 */
void watchdog_feed(osal_task_handle_t handle)
{
    uint32_t index;

    //-------------------------参数检查 begin-------------------------//
    if (s_isWatchdogPaused || (s_watchdogMutex == NULL) || (handle == NULL))
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    if (osal_mutex_take(s_watchdogMutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS)
    {
        return;
    }

    for (index = 0U; index < MAX_WATCHDOG_TASKS; index++)
    {
        if (s_watchdogEntries[index].is_active &&
            (s_watchdogEntries[index].task_handle == handle))
        {
            s_watchdogEntries[index].last_feed_time = osal_task_get_tick_count();
            break;
        }
    }

    osal_mutex_give(s_watchdogMutex);
}

/**
 * @brief 暂停软件超时检测和任务时间戳更新。
 *
 * 步骤：
 *  1. 设置暂停标志；监控线程运行时仍刷新硬件看门狗。
 *
 * @param 无。
 *
 * @return 无。
 *
 */
void watchdog_pause(void)
{
    s_isWatchdogPaused = true;
}

/**
 * @brief 恢复软件超时检测并更新时间戳。
 *
 * 步骤：
 *  1. 清除暂停标志，检查互斥锁是否存在。
 *  2. 获取当前时刻和互斥锁，更新活动表项后释放锁。
 *
 * @param 无。
 *
 * @return 无；互斥锁不存在或获取失败时提前返回。
 *
 */
void watchdog_resume(void)
{
    uint32_t now;
    uint32_t index;

    s_isWatchdogPaused = false;
    if (s_watchdogMutex == NULL)
    {
        return;
    }

    now = osal_task_get_tick_count();
    if (osal_mutex_take(s_watchdogMutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS)
    {
        return;
    }

    for (index = 0U; index < MAX_WATCHDOG_TASKS; index++)
    {
        if (s_watchdogEntries[index].is_active)
        {
            s_watchdogEntries[index].last_feed_time = now;
        }
    }

    osal_mutex_give(s_watchdogMutex);
}

/**
 * @brief 查询软件看门狗暂停状态。
 *
 * 步骤：
 *  1. 读取并返回暂停标志。
 *
 * @param 无。
 *
 * @return bool：true 表示暂停，false 表示未暂停。
 *
 */
bool watchdog_is_paused(void)
{
    return s_isWatchdogPaused;
}

/**
 * @brief 查询初始化时确定的安全模式标志。
 *
 * 步骤：
 *  1. 读取并返回安全模式标志。
 *
 * @param 无。
 *
 * @return bool：true 表示标志已置位，否则为 false。
 *
 */
bool watchdog_is_safe_mode(void)
{
    return s_isSafeMode;
}

/**
 * @brief 清零启动失败计数并保存。
 *
 * 步骤：
 *  1. 计数非零时清零内存计数和备份计数；保留安全模式标志。
 *
 * @param 无。
 *
 * @return 无。
 *
 */
void watchdog_clear_boot_fail_count(void)
{
    if (s_bootFailCount > 0U)
    {
        s_bootFailCount = 0U;
        watchdog_port_boot_fail_write(0U);
        WDOG_LOG("System stable, boot fail count cleared.\r\n");
    }
}

/**
 * @brief 运行看门狗监控线程。
 *
 * 步骤：
 *  1. 初始化服务，循环按配置周期扫描。
 *  2. 未暂停时获取互斥锁，检查活动任务是否超时。
 *  3. 发现超时则延时后请求软件复位，否则刷新硬件看门狗。
 *  4. 暂停时直接刷新硬件看门狗，每轮结束后延时。
 *
 * @param[in] argument : 线程入口参数，当前实现未使用。
 *
 * @return 无；线程持续循环运行。
 *
 */
void server_watchdog_task(void *argument)
{
    (void)argument;
    watchdog_service_init();

    for (;;)
    {
        if (!s_isWatchdogPaused)
        {
            uint32_t now = osal_task_get_tick_count();
            bool isSystemHealthy = true;

            if ((s_watchdogMutex != NULL) &&
                (osal_mutex_take(s_watchdogMutex, OSAL_WAIT_FOREVER) == OSAL_SUCCESS))
            {
                uint32_t index;

                for (index = 0U; index < MAX_WATCHDOG_TASKS; index++)
                {
                    if (s_watchdogEntries[index].is_active)
                    {
                        if ((now - s_watchdogEntries[index].last_feed_time) >
                            s_watchdogEntries[index].timeout_ms)
                        {
                            isSystemHealthy = false;
                            WDOG_LOG("Watchdog Bite! Task '%s' stuck for %lu ms.\r\n",
                                     s_watchdogEntries[index].task_name,
                                     (unsigned long)(now - s_watchdogEntries[index].last_feed_time));
                            break;
                        }
                    }
                }

                osal_mutex_give(s_watchdogMutex);
            }

            if (isSystemHealthy)
            {
                watchdog_port_refresh();
            }
            else
            {
                WDOG_LOG("System resetting...\r\n");
                osal_task_delay(osal_ms_to_ticks(100U));
                watchdog_port_system_reset();
            }
        }
        else
        {

            watchdog_port_refresh();
        }

        osal_task_delay(osal_ms_to_ticks(WATCHDOG_SCAN_PERIOD_MS));
    }
}

//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
