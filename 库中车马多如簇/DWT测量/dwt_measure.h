/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file dwt_measure.h
 * @par dependencies stdint.h, stdio.h（默认打印通道）, CMSIS 器件头
 * @author YA
 * @brief DWT 区间计时：进入记录起点，退出直接 printf。
 * Processing flow: 初始化 -> enter -> 被测代码 -> exit。
 * @version V1.0
 * @note 1 tab == 4 spaces! 单通道，不可并发或嵌套使用。
 *       只依赖 ARM 内核 DWT 周期计数器，可移植到 Cortex-M3/M4/M7/M33
 *       及以上的 ARMv8-M/v8.1-M 内核；M0/M0+/M23 没有 CYCCNT，不支持。
 *****************************************************************************/
#ifndef DWT_MEASURE_H
#define DWT_MEASURE_H

#include <stdint.h>

typedef enum
{
    DWT_MEASURE_OK = 0,
    DWT_MEASURE_NOT_STARTED,
    DWT_MEASURE_CLOCK_ERROR,
    DWT_MEASURE_PRINT_ERROR,
    DWT_MEASURE_NO_COUNTER
} dwt_measure_status_t;

/*
 * 使用：包含本头文件，起点调用 dwt_measure_enter()，
 * 终点调用 dwt_measure_exit()；无需声明变量或手动初始化。
 * exit 立即 printf；打印在结束采样后，不计入本次结果。
 * ISR 中打印仍会延长中断占用，建议低频配对调用。
 * 单通道，不可并发或嵌套；重复 enter 覆盖旧起点。
 *
 * 固定扣除依据：170 MHz 当前工程空区间连续实测 raw=30 cycles，
 * 旧自动校准为 21 cycles，残余 9 cycles，约 0.053 us。
 * 因此采用 30 cycles（约 0.176 us）作为固定估计开销，不再自动校准。
 * cycles=max(raw-30,0)，us=cycles*1000000/SystemCoreClock。
 * 30 是旧构建实测参考，不保证修改代码后或所有调用位置均准确。
 * 更换编译优化、链接布局、Flash 配置或迁移工程后，应再空测验证。
 * 温度通常不直接改变指令周期数，但时钟漂移会影响微秒换算。
 * CYCCNT 每个内核时钟周期加 1，刻度就是 CPU 主频，不是独立时基；
 * 微秒换算只依赖 SystemCoreClock，运行时改主频必须同步更新它。
 * 换内核只影响两处：器件头来源、M7/M33 及以上需先解锁 DWT（LAR）。
 * 区间内中断抢占仍计入结果，固定扣除不保证 0.1 us 绝对精度。
 * DWT 分辨率在 170 MHz 下约 0.00588 us；25.26 秒是单次区间上限，
 * 并非分辨率（32 位周期计数回绕）；测量期间保持时钟固定。
 */
/*
 * 打印分频：每次调用都完整测量并更新最大值，但每 DWT_MEASURE_SAMPLE_DIV
 * 次 exit 才打印一行；打印行里附带上一窗口内的最大周期数与最大微秒数，
 * 打印后窗口最大值清零（想要全局最大值就别清零，见 .c 里的注释）。
 * 未显式定义时取工程配置 CFG_FEATURE_MEASURE_DIV，都没有则按 1（每次都打印）。
 * 20 kHz 电流环中必须分频，否则 RTT 上行缓冲（1 KB）会被打印淹没。
 */
/* 本模板不引入任何工程配置头：需要统一配置时，在包含本头文件前定义
 * CFG_FEATURE_MEASURE_DIV 或 DWT_MEASURE_SAMPLE_DIV。 */

/* 打印通道：工程可改成自己的日志函数（如 RTT 日志），默认走 stdio。 */
#ifndef DWT_MEASURE_PRINTF
#include <stdio.h>
#define DWT_MEASURE_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

#ifndef DWT_MEASURE_SAMPLE_DIV
#ifdef CFG_FEATURE_MEASURE_DIV
#define DWT_MEASURE_SAMPLE_DIV CFG_FEATURE_MEASURE_DIV
#else
#define DWT_MEASURE_SAMPLE_DIV 1U
#endif
#endif

#ifndef DWT_MEASURE_OVERHEAD_CYCLES
#define DWT_MEASURE_OVERHEAD_CYCLES 30U
#endif
/* 首次 enter 仅自动初始化（解锁+启用+自检），不再校准。
 * 返回 DWT_MEASURE_NO_COUNTER 表示内核无可用周期计数器或被锁死。 */
dwt_measure_status_t dwt_measure_enter(void);
/* 扣除空区间基线，更新最大值；未打印时返回 DWT_MEASURE_OK 或 NOT_STARTED。 */
dwt_measure_status_t dwt_measure_exit(void);
#endif
