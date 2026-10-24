/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file project_config.h
 *
 * @par dependencies
 * - board_config.h
 * - feature_config.h
 * - version_config.h
 *
 * @author YA
 *
 * @brief 裸机工程配置的统一入口。
 *
 * Processing flow:
 *
 * 应用、平台和 BSP 通过该文件获得工程配置。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include "board_config.h"
#include "feature_config.h"
#include "version_config.h"

#define CFG_EVENT_QUEUE_DEPTH 8U

#endif /* PROJECT_CONFIG_H */
