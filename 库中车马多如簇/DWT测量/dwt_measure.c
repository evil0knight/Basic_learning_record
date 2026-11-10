/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file dwt_measure.c
 * @par dependencies dwt_measure.h, CMSIS 器件头
 * @author YA
 * @brief 两个函数调用之间的 DWT 周期计时与 0.1 微秒占比统计打印。
 * Processing flow: 解锁并启用计数器 -> 自检 -> 保存起点 -> 读取终点
 *                  -> 按 0.1 us 值计入小表 -> 分频命中时打印占比。
 * @version V1.3
 * @note 1 tab == 4 spaces! 测量包含期间发生的中断抢占时间。
 *       只依赖 ARM 内核的 DWT 周期计数器，不含厂商外设依赖。
 *****************************************************************************/
#include "dwt_measure.h"

//****************************** Includes begin ******************************//
/* 只为 NULL 定义；不依赖 DWT_MEASURE_PRINTF 是否引入 stdio.h。 */
#include <stddef.h>
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

//***************************** Constants begin ******************************//
/* 计数值饱和上限：累计到顶后不再加，避免回绕把占比算错。 */
#define DWT_MEASURE_CNT_MAX 0xFFFFFFFFU
//****************************** Constants end *******************************//

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
/* 小表：0.1 us 值按升序排列，cnt 与 dus 一一对应。 */
static uint32_t g_au32_dus[DWT_MEASURE_SLOT_MAX];
static uint32_t g_au32_cnt[DWT_MEASURE_SLOT_MAX];
static uint32_t g_u32_used;   /* 已占用槽位数 */
static uint32_t g_u32_total;  /* 累计样本数, 占比分母 */
static uint32_t g_u32_other;  /* 未入槽的样本数, 见 other */
static uint32_t g_u32_max_dus;/* 上电以来最慢一次, 0.1 us */
//*************************** Static Variables end ***************************//

//************************ Function Prototypes begin *************************//
//************************* Function Prototypes end **************************//

//************************** Static Functions begin **************************//
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
    /* 第一次打印等满一个分频窗口，避免上电立刻打一行只有 1 个样本的表。 */
    g_u32_print_div = (uint32_t)DWT_MEASURE_SAMPLE_DIV - 1U;
    g_u32_ready = 1U;
    return DWT_MEASURE_OK;
}

/**
 * @brief 把一次测量按 0.1 微秒值归入小表。
 * @param[in] u32_cycles : 已扣除固定基线的周期数。
 * @note 表内按 0.1 us 升序，遇到新值才占位；表满后新值计入 other。
 *       累计不清零，打印只读这张表，所以稀疏路径不会被分频抽掉。
 */
static void dwt_measure_tally(uint32_t u32_cycles)
{
    uint32_t u32_per_us;
    uint32_t u32_us;
    uint32_t u32_rem;
    uint32_t u32_dus;
    uint32_t u32_idx;
    uint32_t u32_move;

    //-------------------------参数检查 begin-------------------------//
    if (0U == SystemCoreClock)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//
    u32_per_us = SystemCoreClock / 1000000U;
    if (0U == u32_per_us)
    {
        u32_per_us = 1U;
    }
    /* 0.1 us 一档：先取整微秒，再用余数补出小数位。
     * 这样绕开 cycles*10 的 32 位溢出，不需要额外的溢出分支。 */
    u32_us = u32_cycles / u32_per_us;
    u32_rem = u32_cycles % u32_per_us;
    u32_dus = (u32_us * 10U) +
        (((u32_rem * 10U) + (u32_per_us / 2U)) / u32_per_us);
    /* 最大值用未量化的原始值，保留 0.1 us 精度；被过滤掉的极端值靠它兜底。 */
    if (u32_dus > g_u32_max_dus)
    {
        g_u32_max_dus = u32_dus;
    }
    /* 线性查表：表按升序，扫到第一个不小于目标值的槽即可停。 */
    u32_idx = 0U;
    while ((u32_idx < g_u32_used) && (g_au32_dus[u32_idx] < u32_dus))
    {
        u32_idx++;
    }
    if ((u32_idx < g_u32_used) && (g_au32_dus[u32_idx] == u32_dus))
    {
        if (DWT_MEASURE_CNT_MAX > g_au32_cnt[u32_idx])
        {
            g_au32_cnt[u32_idx]++;
        }
    }
    else if (DWT_MEASURE_SLOT_MAX > g_u32_used)
    {
        /* 新值按升序插入，后面的槽整体后移一格。 */
        u32_move = g_u32_used;
        while (u32_move > u32_idx)
        {
            g_au32_dus[u32_move] = g_au32_dus[u32_move - 1U];
            g_au32_cnt[u32_move] = g_au32_cnt[u32_move - 1U];
            u32_move--;
        }
        g_au32_dus[u32_idx] = u32_dus;
        g_au32_cnt[u32_idx] = 1U;
        g_u32_used++;
    }
    else
    {
        g_u32_other++;
    }
    g_u32_total++;
}

/**
 * @brief 把计数值换算成千分比，即 0.1% 精度的占比。
 * @param[in] u32_cnt : 该项累计次数；分母取 g_u32_total。
 * @return uint32_t : 千分比，0 表示不足 0.1%。
 * @note 只在打印时算，不进热路径。
 */
static uint32_t dwt_measure_permille(uint32_t u32_cnt)
{
    //-------------------------参数检查 begin-------------------------//
    if (0U == g_u32_total)
    {
        return 0U;
    }
    //--------------------------参数检查 end--------------------------//
    return (uint32_t)(((uint64_t)u32_cnt * 1000ULL) / g_u32_total);
}

/**
 * @brief 打印一个千分比，形如 "49.9%"。
 * @param[in] u32_permille : 千分比，必须非 0。
 * @return int : printf 返回值，负值表示失败。
 */
static int dwt_measure_print_pct(uint32_t u32_permille)
{
    return DWT_MEASURE_PRINTF("%lu.%lu%%",
        (unsigned long)(u32_permille / 10U),
        (unsigned long)(u32_permille % 10U));
}

/**
 * @brief 累加打印结果，任何一次失败都保留负值。
 * @param[in] i_ret : 本次 printf 返回值。
 * @param[in,out] p_i_result : 累计结果。
 * @note 只为让 report 里每个打印点都能写成一行。
 */
static void dwt_measure_err(int i_ret, int *p_i_result)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_i_result)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//
    if (0 > i_ret)
    {
        *p_i_result = i_ret;
    }
}

/**
 * @brief 打印项间分隔符，第一项之前不打印。
 * @param[in,out] p_u32_first : 1 表示还没有打印过任何项。
 * @return int : printf 返回值，负值表示失败。
 * @note 用"先判断再打"代替"打完补空格"，行尾就不会多一个空格。
 */
static int dwt_measure_sep(uint32_t *p_u32_first)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_u32_first)
    {
        return 0;
    }
    //--------------------------参数检查 end--------------------------//
    if (0U == *p_u32_first)
    {
        return DWT_MEASURE_PRINTF(DWT_MEASURE_SEP);
    }
    *p_u32_first = 0U;
    return 0;
}

/**
 * @brief 打印一行占比表，只在打印分频命中时调用。
 * @return int : printf 返回值，负值表示失败。
 * @note 只读统计，不清零。占比不足 0.1% 的槽不单独占一行，计数并入
 *       other，这样整行百分比仍合计 100% 且能看出筛掉了多少。
 *       行尾固定打印上电以来的最大值，被过滤掉的极端值靠它兜底。
 */
static int dwt_measure_report(void)
{
    uint32_t u32_idx;
    uint32_t u32_first;
    uint32_t u32_permille;
    uint32_t u32_hidden;
    uint32_t u32_other;
    int i_result;

    //-------------------------参数检查 begin-------------------------//
    if (0U == g_u32_total)
    {
        return 0;
    }
    //--------------------------参数检查 end--------------------------//
    i_result = 0;
    u32_first = 1U;
    u32_hidden = 0U;
    for (u32_idx = 0U; u32_idx < g_u32_used; u32_idx++)
    {
        u32_permille = dwt_measure_permille(g_au32_cnt[u32_idx]);
        if (0U == u32_permille)
        {
            u32_hidden += g_au32_cnt[u32_idx];
            continue;
        }
        dwt_measure_err(dwt_measure_sep(&u32_first), &i_result);
        dwt_measure_err(DWT_MEASURE_PRINTF("%lu.%luus:",
            (unsigned long)(g_au32_dus[u32_idx] / 10U),
            (unsigned long)(g_au32_dus[u32_idx] % 10U)), &i_result);
        dwt_measure_err(dwt_measure_print_pct(u32_permille), &i_result);
    }
    u32_other = g_u32_other + u32_hidden;
    u32_permille = dwt_measure_permille(u32_other);
    if (0U != u32_permille)
    {
        dwt_measure_err(dwt_measure_sep(&u32_first), &i_result);
        dwt_measure_err(DWT_MEASURE_PRINTF("other:"), &i_result);
        dwt_measure_err(dwt_measure_print_pct(u32_permille), &i_result);
    }
    dwt_measure_err(dwt_measure_sep(&u32_first), &i_result);
    dwt_measure_err(DWT_MEASURE_PRINTF("max:%lu.%luus",
        (unsigned long)(g_u32_max_dus / 10U),
        (unsigned long)(g_u32_max_dus % 10U)), &i_result);
    dwt_measure_err(DWT_MEASURE_PRINTF("\r\n"), &i_result);
    return i_result;
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
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
 * @brief 读取终点并扣除空区间基线，计入小表，按分频打印占比。
 * @return dwt_measure_status_t : 成功、未开始、时钟或打印错误。
 * @note 终点在函数第一句锁存，之后的减法、查表、printf 都不进入本次结果。
 *       注意：这里为了占比必须先查表再打印，尾部耗时比 V1.0 长一些，
 *       debug 场景可接受；实时环路请把 DWT_MEASURE_SAMPLE_DIV 调大。
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
    /* 记账：每次 exit 都进表，不抽样，稀疏路径一次也不会漏。 */
    dwt_measure_tally(u32_cycles);
    /* 打印分频：用减计数代替取模，省掉每周期一次 UDIV。 */
    if (0U != g_u32_print_div)
    {
        g_u32_print_div--;
        return DWT_MEASURE_OK;
    }
    g_u32_print_div = (uint32_t)DWT_MEASURE_SAMPLE_DIV - 1U;
    i_print_result = dwt_measure_report();
    return (0 > i_print_result) ?
        DWT_MEASURE_PRINT_ERROR : DWT_MEASURE_OK;
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
