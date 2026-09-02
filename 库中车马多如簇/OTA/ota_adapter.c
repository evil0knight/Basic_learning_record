#include "ota_adapter.h"
#include <string.h>
#include "ota_config.h"
#include "stm32f4xx_hal.h"
#include "watchdog_adapter_port.h"

/*
 * STM32F411 默认实现
 *
 * 本文件是 OTA 中随芯片/产品变化的"操作"实现。
 * 移植时搜索 TODO 标记即可定位所有要改的位置：
 *
 *   TODO[换芯片]：复位 API / 跳转 API（Flash 读写已由 Flash Wrapper 解耦，不在这里）
 *   TODO[换产品]：UI 交互 / 命令接收 / AES 密钥
 *
 * 分工：
 *   - 复位、跳转（芯片相关）：本文件已按 STM32F411 实现，换芯片才改。
 *   - UI 交互、命令接收（产品相关）：本文件给弱函数空实现，产品层用
 *     同名强函数覆盖。覆盖时对照 OTA/MOC.md 的"APP 端流程"逐项实现即可。
 */

/* ===================== 复位 / 跳转（芯片相关）===================== */

/*
 * 软复位整机。
 *
 * 【流程位置】APP 端第 5 步 OtaEnd 状态，延时 1 秒后被调用。
 * 【作用】OTA 完成后复位，让 BootLoader 上电读标志位续跑状态机。
 * 【产品层】不用改（除非换芯片）。
 */
/* TODO[换芯片]: NVIC_SystemReset 换成目标芯片的复位 API */
void ota_adapter_system_reset(void)
{
    __set_FAULTMASK(1);
    NVIC_SystemReset();
}

/*
 * 跳转到 APP（不可返回）。
 *
 * 【流程位置】BootLoader 端 BootManager_JumpToApp() 里调用。
 * 【作用】BootLoader 校验通过后跳转到 APP 运行区执行。
 * 【参数】addr = APP 运行区地址（OTA_APP_ADDRESS）。
 * 【产品层】不用改（除非换芯片）。
 * 流程：校验栈指针 -> 关中断 -> 设 VTOR -> 关外设 -> 设 MSP -> 跳复位向量。
 */
/* TODO[换芯片]: NVIC_SetVectorTable / RCC_DeInit / __set_MSP / 函数指针跳转
 *              换成目标芯片的向量表设置、外设复位、栈设置与跳转 API */
void ota_adapter_jump_to_app(uint32_t addr)
{
    uint32_t stack_top = (*(__IO uint32_t *)addr);
    uint32_t reset_handler;

    /* 栈指针必须落在 SRAM 区，否则 APP 无效 */
    if ((stack_top & 0x2FFE0000U) != 0x20000000U)
        return;

    __disable_irq();
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, addr & 0xFFFFU);
    RCC_DeInit();
    reset_handler = *(__IO uint32_t *)(addr + 4U);
    __set_MSP(stack_top);

    ((void (*)(void))reset_handler)();
}

/*
 * 跳转前重新初始化看门狗。
 *
 * 【流程位置】BootLoader 端 BootManager_JumpToApp() 的 g_JumpInit 分支里，
 *           跳转 APP 之前调用。
 * 【作用】软复位后 IWDG 仍在运行（IWDG 软复位不清除），跳转前需重新配置
 *        喂狗，否则跳转过程中可能触发看门狗复位。
 * 【实现】直接调库内看门狗 port：watchdog_port_init() 重新配置 IWDG，
 *        watchdog_port_refresh() 喂狗。
 */
void ota_adapter_watchdog_reinit(void)
{
    watchdog_port_init();
    watchdog_port_refresh();
}

/* ===================== UI 交互（产品相关，弱函数）===================== */

/*
 * 写 UI 状态，驱动界面切换。
 *
 * 【流程位置】APP 端状态机里被调 3 次，用 state 区分：
 *   - 第 1 步 WaitReqDownload 开头：state=0（空闲）
 *   - 第 2 步 收到下载命令后：state=1（请求下载确认）
 *   - 第 4 步 WaitReqUpdate 开头：state=2（下载完成待安装）
 *
 * 【参数】state：0=空闲；1=请求下载确认；2=下载完成待安装
 *
 * 【产品层怎么做】强定义覆盖本函数，按 state 切换界面：
 *   - 有屏幕：state=0 回主界面；state=1 显示"是否下载"；state=2 显示"是否安装"。
 *   - 无屏幕（举例）：用 LED 区分，state=0 灭、state=1/2 亮或闪烁。
 *   - 蓝牙/APP：发对应的状态通知。
 */
__attribute__((weak)) void ota_adapter_ui_state_write(uint32_t state)
{
    (void)state;
}

/*
 * 写下载进度。
 *
 * 【流程位置】APP 端第 3 步 OtaDownload 状态，下载开始调 0、结束调 100。
 *
 * 【参数】percent：0~100 下载百分比。
 *
 * 【产品层怎么做】强定义覆盖本函数：
 *   - 有屏幕：刷新进度条/百分比数字。
 *   - 无屏幕（举例）：忽略，或用一个 LED 常亮表示"正在下载"。
 */
__attribute__((weak)) void ota_adapter_ui_progress_write(uint8_t percent)
{
    (void)percent;
}

/*
 * 读用户确认结果（阻塞轮询由 service 层 wait_confirmation() 做）。
 *
 * 【流程位置】APP 端被 wait_confirmation() 轮询调用，出现在两步：
 *   - 第 2 步 等确认下载；
 *   - 第 4 步 等确认安装。
 *
 * 【返回值】0=等待中（继续轮询）；1=用户确认；2=用户取消。
 *
 * 【产品层怎么做】强定义覆盖本函数，返回当前确认状态：
 *   - 按键（举例）：检测按键按下，按下返回 1，否则返回 0。
 *   - 触屏：读按钮点击状态。
 *   - 蓝牙/APP：读远端下发的确认指令。
 */
__attribute__((weak)) uint8_t ota_adapter_ui_confirm_read(void)
{
    return 0U;
}

/*
 * 写确认状态（清空/预置）。
 *
 * 【流程位置】APP 端第 4 步 WaitReqUpdate 开头，在 wait_confirmation() 之前调用，
 *           用来清掉上一次确认留下的状态，避免旧状态被误判成本次确认。
 *
 * 【参数】value：0=清为等待；1=置为确认；2=置为取消。
 *
 * 【产品层怎么做】强定义覆盖本函数：
 *   - 按键（举例）：清按下标志，并把"上次电平"同步成当前电平。
 *   - 触屏/蓝牙：清对应标志位。
 */
__attribute__((weak)) void ota_adapter_ui_confirm_write(uint8_t value)
{
    (void)value;
}

/* ===================== 下载命令接收（产品相关，弱函数）===================== */

/*
 * 阻塞等待 OTA 下载命令。
 *
 * 【流程位置】APP 端第 1 步 WaitReqDownload 状态，循环调本函数等触发。
 *
 * 【返回值】收到下载命令返回 1（进入第 2 步等确认）；没收到返回 0（继续等）。
 *
 * 【产品层怎么做】强定义覆盖本函数，实现"怎么算收到下载命令"：
 *   - UART 命令：UART 空闲中断收 4 字节，匹配 0x11 0x22 0x33 返回 1。
 *   - 按键（举例）：按键按下返回 1。
 *   - 蓝牙/APP：收到远端下发"开始下载"指令返回 1。
 */
__attribute__((weak)) int ota_adapter_wait_download_request(void)
{
    return 0;
}
