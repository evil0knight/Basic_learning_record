/******************************************************************************
 * 看门狗监控服务（移植自智能手表工程）
 *
 * 来源：ec_s100_watch_V2.5_T3605
 *       01_Service/service/service_watchdog_monitor/service_watchdog_monitor.c
 *
 * 分层说明：
 *   - 本文件：软件看门狗表 + 复位计数 + 安全模式（芯片无关，移植不改）
 *   - watchdog_adapter_port：硬件 IWDG / 备份寄存器 / 复位原因（换芯片改它）
 *   - osal：OS 抽象层（task handle / tick / mutex，换 RTOS 改 osal 实现）
 *****************************************************************************/

#include "service_watchdog_monitor.h"
#include "watchdog_config.h"
#include "watchdog_adapter_port.h"
#include "osal.h"
#include <string.h>

/* 日志：手表工程用 DEBUG_OUT(...)，这里提供可替换宏，移植时接 elog/printf */
#ifndef WDOG_LOG
#define WDOG_LOG(fmt, ...) ((void)0)
#endif

typedef struct
{
    osal_task_handle_t task_handle;
    uint32_t last_feed_time;
    uint32_t timeout_ms;
    char task_name[WATCHDOG_TASK_NAME_LEN];
    bool is_active;
} watchdog_entry_t;

static watchdog_entry_t s_watchdogEntries[MAX_WATCHDOG_TASKS];
static osal_mutex_handle_t s_watchdogMutex = NULL;
static volatile bool s_isWatchdogPaused = false;
static uint32_t s_bootFailCount = 0U;
static bool s_isSafeMode = false;
static bool s_isWatchdogInitialized = false;

static void watchdog_entry_reset(watchdog_entry_t *p_entry);
static void watchdog_entries_init(void);

/* 复位一个表项到默认状态 */
static void watchdog_entry_reset(watchdog_entry_t *p_entry)
{
    if (p_entry == NULL)
    {
        return;
    }

    p_entry->task_handle = NULL;
    p_entry->last_feed_time = 0U;
    p_entry->timeout_ms = WATCHDOG_TIMEOUT_MS;
    p_entry->task_name[0] = '\0';
    p_entry->is_active = false;
}

/* 清空整张软件看门狗表 */
static void watchdog_entries_init(void)
{
    uint32_t index;

    for (index = 0U; index < MAX_WATCHDOG_TASKS; index++)
    {
        watchdog_entry_reset(&s_watchdogEntries[index]);
    }
}

/* 初始化软件看门狗状态与复位计数 */
void watchdog_service_init(void)
{
    if (s_isWatchdogInitialized)
    {
        return;
    }

    /* 硬件初始化：手表在 main.c 的 MX_IWDG_Init() 完成，这里收敛进 service 便于自包含 */
    watchdog_port_init();

    if ((s_watchdogMutex == NULL) &&
        (osal_mutex_create(&s_watchdogMutex) != OSAL_SUCCESS))
    {
        WDOG_LOG("Watchdog mutex create failed!\r\n");
        return;
    }

    watchdog_entries_init();

    /* 用备份寄存器统计连续 IWDG 复位次数（掉电保留） */
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

/* 注册一个任务到软件看门狗表 */
void watchdog_register(osal_task_handle_t handle, uint32_t timeout_ms, const char *name)
{
    uint32_t index;

    if ((s_watchdogMutex == NULL) || (handle == NULL) || (name == NULL))
    {
        return;
    }

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

/* 从软件看门狗表注销一个任务 */
void watchdog_unregister(osal_task_handle_t handle)
{
    uint32_t index;

    if ((s_watchdogMutex == NULL) || (handle == NULL))
    {
        return;
    }

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

/* 喂一个已注册任务对应的表项 */
void watchdog_feed(osal_task_handle_t handle)
{
    uint32_t index;

    if (s_isWatchdogPaused || (s_watchdogMutex == NULL) || (handle == NULL))
    {
        return;
    }

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

/* 暂停软件超时检测 */
void watchdog_pause(void)
{
    s_isWatchdogPaused = true;
}

/* 恢复软件超时检测，并重置所有表项时间戳避免误报 */
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

bool watchdog_is_paused(void)
{
    return s_isWatchdogPaused;
}

bool watchdog_is_safe_mode(void)
{
    return s_isSafeMode;
}

/* 清除 boot fail 计数 */
void watchdog_clear_boot_fail_count(void)
{
    if (s_bootFailCount > 0U)
    {
        s_bootFailCount = 0U;
        watchdog_port_boot_fail_write(0U);
        WDOG_LOG("System stable, boot fail count cleared.\r\n");
    }
}

/*
 * 看门狗监控线程：
 *   每 WATCHDOG_SCAN_PERIOD_MS 扫一次表，全部健康才喂硬件 IWDG；
 *   任一任务超时则软复位；暂停期间直接喂硬件狗（睡眠兜底）。
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
            /* 低功耗暂停期间仍要喂硬件狗，否则睡眠中被 IWDG 复位 */
            watchdog_port_refresh();
        }

        osal_task_delay(osal_ms_to_ticks(WATCHDOG_SCAN_PERIOD_MS));
    }
}
