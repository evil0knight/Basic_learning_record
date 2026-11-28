/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file can_port_config.h
 *
 * @par dependencies
 * - can_port.h
 *
 * @author YA
 *
 * @brief CAN 实例静态配置表，仅由 can_port.c 包含。
 *
 * Processing flow:
 *
 * 参数检查 -> 调用 HAL CAN 或分发回调 -> 状态码转换 -> 返回。
 *
 * @version V1.0 2026-11-23
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef CAN_PORT_CONFIG_H
#define CAN_PORT_CONFIG_H

//****************************** Includes begin ******************************//
#include "can_port.h"
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
/* 取消注释并替换占位项；默认不绑定任何硬件。 */
static const st_can_config_t g_can_configs[CORE_CAN_MAX] =
{
    {0} /* C99 合法的零初始化占位项，启用下方配置时删除本行。 */
    // [CORE_CAN1] =
    // {
    //     .p_handle = &hfdcan1, /* bxCAN 后端改为 &hcan1。 */
    //     .timeout  = CORE_CAN_TX_TIMEOUT_MS
    // }
};
//*************************** Static Variables end ***************************//

#endif /* CAN_PORT_CONFIG_H */

//******************************* End of File ********************************//

