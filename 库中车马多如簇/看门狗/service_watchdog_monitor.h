/*****************************************************************************
 * Copyright (C) 2026 看门狗模块贡献者。
 *
 * 保留所有权利。
 *
 * @file service_watchdog_monitor.h
 *
 * @par dependencies
 * - osal.h
 * - stdbool.h
 * - stdint.h
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
 *
 ****************************************************************************/

#ifndef SERVICE_WATCHDOG_MONITOR_H
#define SERVICE_WATCHDOG_MONITOR_H

//****************************** Includes begin ******************************//
#include "osal.h"
#include <stdbool.h>
#include <stdint.h>
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//

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
void server_watchdog_task(void *argument);

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
void watchdog_service_init(void);

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
void watchdog_register(osal_task_handle_t handle, uint32_t timeout_ms, const char *name);

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
void watchdog_unregister(osal_task_handle_t handle);

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
void watchdog_feed(osal_task_handle_t handle);

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
void watchdog_pause(void);

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
void watchdog_resume(void);

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
bool watchdog_is_paused(void);

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
bool watchdog_is_safe_mode(void);

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
void watchdog_clear_boot_fail_count(void);

//************************* Function Prototypes end **************************//

#endif

//******************************* End of File ********************************//
