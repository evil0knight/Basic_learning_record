/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file dwt_measure.c
 * @par dependencies dwt_measure.h, CMSIS 器件头
 * @author YA
 * @brief 两个函数调用之间的 DWT 周期计时与直接打印。
 * Processing flow: 解锁并启用计数器 -> 自检 -> 保存起点 -> 读取终点 -> 打印。
 * @version V1.0
 * @note 1 tab == 4 spaces! 测量包含期间发生的中断抢占时间。
 *       只依赖 ARM 内核的 DWT 周期计数器，不含厂商外设依赖。
 *****************************************************************************/
#include "dwt_measure.h"

//****************************** Includes begin ******************************//
/* CMSIS 器件头是 DWT / CoreDebug / SystemCoreClock 的唯一来源。
 * 工程已先行包含时可省略；需要显式指定时在编译选项里定义，例如
 * -DDWT_MEASURE_DEVICE_HEADER='"stm32h7xx.h"' */
#ifdef DWT_MEASURE_DEVICE_HEADER
#include DWT_MEASURE_DEVICE_HEADER
#endif
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
/* CoreSight 软件锁解锁键：Cortex-M7/M33 及以上默认锁定 DWT 寄存器。 */
#define DWT_MEASURE_LAR_KEY 0xC5ACCE55U
//******************************* Defines end ********************************//

//************************ Compile-time Checks begin *************************//
#ifndef DWT_CTRL_CYCCNTENA_Msk
#error "缺少 CMSIS 器件头: 请先包含器件头, 或定义 DWT_MEASURE_DEVICE_HEADER"
#endif
#if defined(__CORTEX_M) && ((__CORTEX_M == 0U) || (__CORTEX_M == 23U))
#error "Cortex-M0/M0+/M23 无 DWT 周期计数器, 请改用 SysTick 或外设定时器"
#endif
//************************* Compile-time Checks end **************************//

//************************** Static Variables begin **************************//
static uint32_t g_u32_ready;
static uint32_t g_u32_active;
static uint32_t g_u32_start;
static uint32_t g_u32_print_div;
static uint32_t g_u32_max;
//*************************** Static Variables end ***************************//

/**
 * @brief 解锁并启用 DWT 周期计数器，并自检其确实在计数。
 * @return dwt_measure_status_t : 初始化状态。
 * @note 不自动校准、不打印、不清零共享计数器。
 *       解锁按内核版本判断，覆盖 M7/M33/M35P/M55/M85。
 */
static dwt_measure_status_t dwt_measure_prepare(void)
{
    uint32_t u32_first;
    uint32_t u32_second;
    uint32_t u32_try;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
#if defined(__CORTEX_M) && ((__CORTEX_M == 7U) || (__CORTEX_M >= 33U))
    DWT->LAR = DWT_MEASURE_LAR_KEY;
#endif
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB();
    __ISB();
    //-------------------------参数检查 begin-------------------------//
    if (0U == SystemCoreClock)
    {
        return DWT_MEASURE_CLOCK_ERROR;
    }
    //--------------------------参数检查 end--------------------------//
#ifdef DWT_CTRL_NOCYCCNT_Msk
    if (0U != (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk))
    {
        return DWT_MEASURE_NO_COUNTER;
    }
#endif
    if (0U == (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk))
    {
        return DWT_MEASURE_NO_COUNTER;
    }
    /* 计数自检：两次读值必须不同，否则说明被锁定或未实际运行。 */
    u32_first = DWT->CYCCNT;
    u32_second = u32_first;
    for (u32_try = 0U; (u32_try < 8U) && (u32_second == u32_first); u32_try++)
    {
        u32_second = DWT->CYCCNT;
    }
    if (u32_second == u32_first)
    {
        return DWT_MEASURE_NO_COUNTER;
    }
    g_u32_ready = 1U;
    return DWT_MEASURE_OK;
}

/**
 * @brief 首次自动启用 DWT，之后保存起点；重复进入覆盖旧起点。
 * @return dwt_measure_status_t : 成功或时钟/计数器错误。
 * @note 禁止内联；接口或编译布局变化后需重新验证固定基线。
 */
__NOINLINE dwt_measure_status_t dwt_measure_enter(void)
{
    if (0U == g_u32_ready)
    {
        dwt_measure_status_t e_status = dwt_measure_prepare();

        if (DWT_MEASURE_OK != e_status)
        {
            return e_status;
        }
    }
    g_u32_active = 1U;
    __COMPILER_BARRIER();
    g_u32_start = DWT->CYCCNT;
    __COMPILER_BARRIER();
    return DWT_MEASURE_OK;
}

/**
 * @brief 打印一行统计，只在打印分频命中时调用。
 * @param[in] u32_raw : 未扣基线的原始周期数。
 * @param[in] u32_cycles : 扣除基线后的周期数。
 * @return int : printf 返回值，负值表示失败。
 * @note 单独成函数，避免 64 位除法把 exit 的寄存器压力传给热路径。
 */
__NOINLINE static int dwt_measure_report(uint32_t u32_raw, uint32_t u32_cycles)
{
    uint64_t u64_ns;
    uint64_t u64_max_ns;

    u64_ns = ((uint64_t)u32_cycles * 1000000000ULL) / SystemCoreClock;
    u64_max_ns = ((uint64_t)g_u32_max * 1000000000ULL) / SystemCoreClock;
    DWT_MEASURE_PRINTF(
        "DWT_MEASURE raw=%lu overhead=%lu cycles=%lu us=%lu.%03lu "
        "max=%lu max_us=%lu.%03lu\r\n",
        (unsigned long)u32_raw, (unsigned long)DWT_MEASURE_OVERHEAD_CYCLES,
        (unsigned long)u32_cycles, (unsigned long)(u64_ns / 1000ULL),
        (unsigned long)(u64_ns % 1000ULL),
        (unsigned long)g_u32_max, (unsigned long)(u64_max_ns / 1000ULL),
        (unsigned long)(u64_max_ns % 1000ULL));
    return 0;
}

/**
 * @brief 读取终点并扣除空区间基线，更新窗口最大值，按分频打印。
 * @return dwt_measure_status_t : 成功、未开始、时钟或打印错误。
 * @note 终点在函数第一句锁存，之后的减法、比较、printf 都不进入本次结果。
 */
__NOINLINE dwt_measure_status_t dwt_measure_exit(void)
{
    uint32_t u32_end = DWT->CYCCNT;
    uint32_t u32_raw, u32_cycles;
    int i_print_result;

    __COMPILER_BARRIER();
    if (0U == g_u32_active)
    {
        return DWT_MEASURE_NOT_STARTED;
    }
    u32_raw = u32_end - g_u32_start;
    g_u32_active = 0U;
    if (0U == SystemCoreClock)
    {
        return DWT_MEASURE_CLOCK_ERROR;
    }
    /* 流水线波动可能令空区间小于基线，饱和为零，避免下溢。 */
    u32_cycles = (u32_raw > DWT_MEASURE_OVERHEAD_CYCLES) ?
        (u32_raw - DWT_MEASURE_OVERHEAD_CYCLES) : 0U;
    /* 最大值统计：只用已算好的周期数，终点早已锁存，不改变本次结果。 */
    if (u32_cycles > g_u32_max)
    {
        g_u32_max = u32_cycles;
    }
    /* 打印分频：用减计数代替取模，省掉每周期一次 UDIV。 */
    if (0U != g_u32_print_div)
    {
        g_u32_print_div--;
        return DWT_MEASURE_OK;
    }
    g_u32_print_div = (uint32_t)DWT_MEASURE_SAMPLE_DIV - 1U;
    i_print_result = dwt_measure_report(u32_raw, u32_cycles);
    /* 窗口最大值清零；要上电以来的全局最大值就注释掉这一行。 */
    g_u32_max = 0U;
    return (0 > i_print_result) ?
        DWT_MEASURE_PRINT_ERROR : DWT_MEASURE_OK;
}
