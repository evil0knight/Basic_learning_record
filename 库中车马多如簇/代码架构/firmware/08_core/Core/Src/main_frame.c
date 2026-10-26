/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file main_frame.c
 *
 * @par dependencies
 * - 01_app/app_init/user_init.h
 * - 01_app/app_init/platform_io_register/user_periph_setup.h
 * - osal_task.h
 *
 * @author YA
 *
 * @brief RTOS 框架入口参考，供手动合入生成的 main.c。
 *
 * Processing flow:
 *
 * 硬件及内核准备 -> 外设注册 -> 创建初始化任务 -> 启动调度器。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 * @note 本文件排除编译；合并完成后可手动删除。
 * @note 工程根目录及 OSAL 公共头文件目录需加入头文件搜索路径。
 *
 *****************************************************************************/
#include "01_app/app_init/user_init.h"
#include "01_app/app_init/platform_io_register/user_periph_setup.h"
#include "osal_task.h"

/**
 * @brief 展示 RTOS 应用入口接入顺序。
 *
 * Steps:
 *  1. 在正式 main.c 中保留 HAL、时钟、外设及必要的内核初始化。
 *  2. 注册外设并创建应用初始化任务。
 *  3. 启动调度器，由初始化任务创建资源和业务任务。
 *
 * @param[in] None
 * @return int : 调度器异常返回时返回非零值。
 */
int main(void)
{
    /* 在此之前保留目标工程的硬件及必要的内核初始化。 */
    app_periph_init();
    UserAppTask_Init();

    /* 与生成工程已有的调度器启动调用合并，只保留一次启动。 */
    (void)osal_task_start_scheduler();

    return 1;
}
