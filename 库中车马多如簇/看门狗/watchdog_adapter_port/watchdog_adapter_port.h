#ifndef WATCHDOG_ADAPTER_PORT_H
#define WATCHDOG_ADAPTER_PORT_H

#include <stdint.h>
#include <stdbool.h>

/*
 * 硬件适配层（port）
 *
 * 这是整个看门狗库中唯一随 MCU 变化的文件组。
 * 换芯片时只需重写对应的 watchdog_adapter_port.c，service_watchdog_monitor.c
 * 无需任何改动。
 *
 * 各接口与 STM32F411 手表的原始调用的对应关系见 .c 文件头部注释。
 */

/* 初始化硬件看门狗（IWDG）并准备备份寄存器访问 */
void watchdog_port_init(void);

/* 喂硬件看门狗（等价 HAL_IWDG_Refresh） */
void watchdog_port_refresh(void);

/* 读取掉电保留的“连续 IWDG 复位次数” */
uint32_t watchdog_port_boot_fail_read(void);

/* 写入掉电保留的“连续 IWDG 复位次数” */
void watchdog_port_boot_fail_write(uint32_t count);

/* 判断本次复位是否由 IWDG 引起，并清除复位标志 */
bool watchdog_port_is_wdg_reset(void);

/* 软件复位整机（等价 NVIC_SystemReset） */
void watchdog_port_system_reset(void);

#endif /* WATCHDOG_ADAPTER_PORT_H */
