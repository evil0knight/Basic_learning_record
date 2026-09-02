#ifndef OTA_ADAPTER_H
#define OTA_ADAPTER_H

#include <stdint.h>

/*
 * OTA 底层适配层（adapter）
 *
 * 这是 OTA 中随芯片/产品变化的"操作"实现层。
 * 注意：Flash 读写已由 Flash Wrapper 解耦，adapter 不再重复封装。
 * adapter 只保留两类东西：
 *   1) 芯片内核操作：复位、跳转（无 Wrapper，必须在这里实现）；
 *   2) 产品交互：UI、命令接收（弱函数，产品层强定义覆盖）。
 *
 * 换芯片：重写 ota_adapter.c 里的复位、跳转。
 * 换产品：覆盖下面的 UI 交互、命令接收弱函数；队列/互斥量在目标工程资源表
 *         申请、通过 ota_config.h 的宏映射；AES 密钥/IV 也在 ota_config.h 配置。
 *
 * ===== 产品层需要覆盖的弱函数（对应 APP 端流程）=====
 * 走完整个 OTA 流程，产品层要覆盖下面 5 个弱函数（复位/跳转已实现，不用改）：
 *
 *   流程第1步 WaitReqDownload（等下载命令）:
 *     ota_adapter_ui_state_write(0)      提示"空闲"
 *     ota_adapter_wait_download_request() 收到下载命令 -> 返回 1
 *
 *   流程第2步 等用户确认下载:
 *     ota_adapter_ui_state_write(1)      提示"是否下载"
 *     ota_adapter_ui_confirm_read()      用户确认 -> 返回 1
 *
 *   流程第3步 OtaDownload（下载中）:
 *     ota_adapter_ui_progress_write(0/100) 刷新进度
 *
 *   流程第4步 WaitReqUpdate（等确认安装）:
 *     ota_adapter_ui_state_write(2)      提示"是否安装"
 *     ota_adapter_ui_confirm_write(0)    清上次确认状态
 *     ota_adapter_ui_confirm_read()      用户确认 -> 返回 1
 *
 *   流程第5步 OtaEnd（复位）:
 *     ota_adapter_system_reset()         软复位（芯片相关，已实现）
 */

/* ===================== 复位 / 跳转（芯片相关）===================== */

/*
 * 软复位整机。
 * 【流程位置】APP 端第 5 步 OtaEnd 状态调用。
 * 【作用】OTA 完成后复位，让 BootLoader 上电读标志位续跑状态机。
 */
void ota_adapter_system_reset(void);

/*
 * 跳转到 APP（不可返回）。
 * @param addr  APP 运行区地址（OTA_APP_ADDRESS）
 * 【流程位置】BootLoader 端 BootManager_JumpToApp() 调用。
 * 流程：校验栈指针 -> 关中断 -> 设 VTOR -> 关外设 -> 设 MSP -> 跳复位向量。
 */
void ota_adapter_jump_to_app(uint32_t addr);

/*
 * 跳转前重新初始化看门狗。
 * 【流程位置】BootLoader 端 g_JumpInit 分支里，跳转 APP 前调用。
 * 【作用】软复位后 IWDG 仍在运行，跳转前需重新配置喂狗，否则跳转中可能复位。
 * 【实现】直接调库内看门狗 port（watchdog_port_init + watchdog_port_refresh）。
 */
void ota_adapter_watchdog_reinit(void);

/* ===================== UI 交互（产品相关，弱函数）===================== */

/*
 * 写 UI 状态，驱动界面切换。
 *
 * 【流程位置】APP 端状态机里被调 3 次：
 *   - 第 1 步 WaitReqDownload 开头：state=0（空闲）
 *   - 第 2 步 收到下载命令后：state=1（请求下载确认）
 *   - 第 4 步 WaitReqUpdate 开头：state=2（下载完成待安装）
 *
 * @param state 0=空闲；1=请求下载确认；2=下载完成待安装
 *
 * 【产品层怎么做】强定义覆盖本函数，按 state 切换界面/指示。
 */
void ota_adapter_ui_state_write(uint32_t state);

/*
 * 写下载进度。
 *
 * 【流程位置】APP 端第 3 步 OtaDownload 状态，下载开始调 0、结束调 100。
 * @param percent 0~100 下载百分比。
 * 【产品层怎么做】强定义覆盖本函数刷新进度显示。
 */
void ota_adapter_ui_progress_write(uint8_t percent);

/*
 * 读用户确认结果（阻塞轮询由 service 层 wait_confirmation() 做）。
 *
 * 【流程位置】APP 端被 wait_confirmation() 轮询，出现在两步：
 *   - 第 2 步 等确认下载；
 *   - 第 4 步 等确认安装。
 *
 * @return 0=等待中；1=用户确认；2=用户取消
 * 【产品层怎么做】强定义覆盖本函数，返回当前确认状态。
 */
uint8_t ota_adapter_ui_confirm_read(void);

/*
 * 写确认状态（清空/预置）。
 *
 * 【流程位置】APP 端第 4 步 WaitReqUpdate 开头，在 wait_confirmation() 之前调用，
 *           清掉上一次确认留下的状态。
 * @param value 0=清为等待；1=置为确认；2=置为取消
 * 【产品层怎么做】强定义覆盖本函数，清对应标志位。
 */
void ota_adapter_ui_confirm_write(uint8_t value);

/* ===================== 下载命令接收（产品相关，弱函数）===================== */

/*
 * 阻塞等待 OTA 下载命令。
 *
 * 【流程位置】APP 端第 1 步 WaitReqDownload 状态，循环调本函数等触发。
 * @return 收到下载命令返回 1、否则 0。
 * 【产品层怎么做】强定义覆盖本函数，实现"怎么算收到下载命令"。
 */
int ota_adapter_wait_download_request(void);

#endif /* OTA_ADAPTER_H */
