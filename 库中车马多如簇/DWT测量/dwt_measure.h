/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file dwt_measure.h
 * @par dependencies stdint.h, stdio.h（默认打印通道）, CMSIS 器件头
 * @author YA
 * @brief DWT 区间计时：进入记录起点，退出按 0.1 微秒值统计并分频打印占比。
 * Processing flow: 初始化 -> enter -> 被测代码 -> exit -> 分频打印。
 * @version V1.3
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
 * exit 第一句锁存 CYCCNT，之后的记账与打印都不影响本次测量结果。
 * ISR 中打印仍会延长中断占用，debug 场景可以接受，实时环路请加大分频。
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
 * 占比统计：每次 exit 都按"0.1 微秒值"归入小表，不抽样——分频只决定多久
 * 倒一次表，不影响哪个样本被记录。小表最多 DWT_MEASURE_SLOT_MAX(16) 种
 * 不同值，遇到新值才占位，按 0.1 us 升序排列，累计不清零。
 * 打印一行形如：5.0us:49.9%    5.5us:24.9%    other:0.2%    max:9.5us
 * 占比不足 0.1% 的槽不单独占一行，计数并入 other，整行仍合计 100%；
 * other 另外收纳表满后没能入槽的样本。
 * 行尾固定打印上电以来最慢的一次（max），0.1 us 精度、不受过滤影响，
 * 所以被筛掉的极端值仍能看到。
 * 被测代码只有几条固定路径，路径数就是占用的格数，16 格够用。
 * 0.1 us 一档 = 170 MHz 下 17 个周期，比指令级抖动粗，
 * 所以同一个路径的样本不会散成一堆相邻格。
 * 真出现 other 且占比不小，说明路径比预想的多，再抬 SLOT_MAX。
 * 占比低于 0.1% 打 "<0.1%"，避免稀疏路径显示成 0.0% 被误认为没测到。
 * 未显式定义分频时取工程配置 CFG_FEATURE_MEASURE_DIV，都没有则按 10000，
 * 20 kHz 电流环下约 500 ms 打一行。20 kHz 不分频会把 RTT 上行缓冲（1 KB）
 * 淹没，且打印本身会拖住实时环路。
 */
/* 本模板不引入任何工程配置头：需要统一配置时，在包含本头文件前定义
 * CFG_FEATURE_MEASURE_DIV 或 DWT_MEASURE_SAMPLE_DIV。 */

/* 打印通道：工程可改成自己的日志函数（如 RTT 日志），默认走 stdio。 */
#ifndef DWT_MEASURE_PRINTF
#include <stdio.h>
#define DWT_MEASURE_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

/* 项与项之间的分隔符：默认 4 个空格，窄体字终端上看不清就再加。 */
#ifndef DWT_MEASURE_SEP
#define DWT_MEASURE_SEP "    "
#endif

/* 小表槽位数：能记住"多少种不同的 0.1 us 值"，满了以后新值归入 other。
 * 每个槽 8 字节（4 字节值 + 4 字节计数），16 槽共 128 字节。 */
#ifndef DWT_MEASURE_SLOT_MAX
#define DWT_MEASURE_SLOT_MAX 16U
#endif

#ifndef DWT_MEASURE_SAMPLE_DIV
#ifdef CFG_FEATURE_MEASURE_DIV
#define DWT_MEASURE_SAMPLE_DIV CFG_FEATURE_MEASURE_DIV
#else
/* 10000 次 ≈ 20 kHz 下 500 ms 一次；改这里按"调用次数"算，不是时间。 */
#define DWT_MEASURE_SAMPLE_DIV 10000U
#endif
#endif

#ifndef DWT_MEASURE_OVERHEAD_CYCLES
#define DWT_MEASURE_OVERHEAD_CYCLES 30U
#endif
/* 首次 enter 仅自动初始化（解锁+启用+自检），不再校准。
 * 返回 DWT_MEASURE_NO_COUNTER 表示内核无可用周期计数器或被锁死。 */
dwt_measure_status_t dwt_measure_enter(void);
/* 扣除空区间基线后计入小表；分频命中时打印占比。
 * 未打印时返回 DWT_MEASURE_OK 或 NOT_STARTED。 */
dwt_measure_status_t dwt_measure_exit(void);
#endif
