#include "watchdog_adapter_port.h"
#include "stm32f4xx_hal.h"

/*
 * STM32F411 IWDG + RTC 备份寄存器 实现
 *
 * 与手表工程 ec_s100_watch_V2.5_T3605 的对应关系：
 *   - watchdog_port_init()          <-> Core/Src/iwdg.c 的 MX_IWDG_Init()
 *   - watchdog_port_refresh()       <-> HAL_IWDG_Refresh(&hiwdg)
 *   - boot_fail_read/write()        <-> HAL_RTCEx_BKUPRead/Write(&hrtc, RTC_BKP_DR0, ...)
 *   - watchdog_port_is_wdg_reset()  <-> RCC->CSR & RCC_CSR_IWDGRSTF + __HAL_RCC_CLEAR_RESET_FLAGS()
 *   - watchdog_port_system_reset()  <-> NVIC_SystemReset()
 *
 * 移植到其它芯片时，重写本文件即可：
 *   - 若目标芯片无“备份寄存器”，可用内部 Flash 模拟（注意擦写寿命），
 *     或用外挂 EEPROM 替代 boot_fail_read/write 的实现。
 */

static IWDG_HandleTypeDef s_hiwdg;

void watchdog_port_init(void)
{
    /* 使能 PWR 时钟并开放备份域访问（写 RTC 备份寄存器前必须） */
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    /*
     * LSI 约 32 kHz，64 分频后 500 Hz，重装值 2500 -> 约 5 秒硬件超时。
     * 喂狗周期务必远小于 5 秒，并考虑 LSI 实际频率偏差（17~47 kHz）。
     */
    s_hiwdg.Instance = IWDG;
    s_hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    s_hiwdg.Init.Reload = 2500U;
    if (HAL_IWDG_Init(&s_hiwdg) != HAL_OK)
    {
        /* 手表工程此处走 Error_Handler() */
    }
}

void watchdog_port_refresh(void)
{
    HAL_IWDG_Refresh(&s_hiwdg);
}

uint32_t watchdog_port_boot_fail_read(void)
{
    /* RTC->BKP0R 即 RTC_BKP_DR0，位于 VBAT 备份域，掉电后由电池维持 */
    return RTC->BKP0R;
}

void watchdog_port_boot_fail_write(uint32_t count)
{
    /* D_BP 位开放备份域写访问（等价 HAL_RTCEx_BKUPWrite 内部行为） */
    PWR->CR |= PWR_CR_DBP;
    RTC->BKP0R = count;
}

bool watchdog_port_is_wdg_reset(void)
{
    bool is_wdg_reset = ((RCC->CSR & RCC_CSR_IWDGRSTF) != 0U);

    /* 清除所有复位标志，避免下次启动误判 */
    __HAL_RCC_CLEAR_RESET_FLAGS();

    return is_wdg_reset;
}

void watchdog_port_system_reset(void)
{
    NVIC_SystemReset();
}
