/****************************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_led_handle.h
 *
 * @par dependencies
 * - bsp_led_driver.h
 *
 * @author yan gao
 *
 * @brief Provide the handler APIs of LEDs - two-phase init with registration.
 *
 * Processing flow:
 *
 *  1. handler_led_inst()  - phase 1: set up time/OS interfaces
 *  2. handler_led_register() - phase 2: register LED driver instance
 *
 * @version V1.0 2026-6-28
 *
 * @note 1 tab == 4 spaces!
 *
 ****************************************************************************************/
#ifndef BSP_LED_HANDLE_H
#define BSP_LED_HANDLE_H
// ================================= include ==================================
#include <stdint.h>
#include <stdio.h>
#include "bsp_led_driver.h"
// =============================== include end ================================

// ================================== Define ==================================
typedef enum {
    LED_HANDLER_INIT_STATUS_NO   =   0,
    LED_HANDLER_INIT_STATUS_OK   =   1,
} LED_HANDLER_INIT_STATUS_t;
// =========================== handler init status ===========================

typedef enum {
    HANDLER_OK              =   0,
    HANDLER_ERROR           =   1,
    HANDLER_ERROR_TIMEOUT   =   2,
    HANDLER_ERROR_RESOURCE  =   3,
    HANDLER_ERROR_PARAMETER =   4,
    HANDLER_ERROR_NO_MEMORY =   5,
    HANDLER_ERROR_ISR       =   6,
    HANDLER_RECEIVED        =   0XFF,
} LED_HANDLER_STATUS_t;
// =========================== handler status codes ==========================

typedef enum {
    LED_1 = 0,
    LED_2,
    LED_3,
    LED_4,
    LED_5,
    LED_6,
    LED_7,
    LED_8,
    LED_9,
    LED_10,
    MAX_INSTANCE_NUMBER,
    LED_INDEX_INVALID = 0xFF,
} LED_INDEX_T;
// ============================= led index define ============================
typedef struct
{
    uint32_t led_instance_num;                               // Number of instances
    bsp_led_driver_t *led_instance_group[MAX_INSTANCE_NUMBER];// Array of instance pointers
}instance_registered_t;
// ========================== registered instances ===========================

#ifdef  OS_SUPPORTING
typedef struct HANDLER_os_delay_t {
    LED_HANDLER_STATUS_t    (*pf_os_delay_ms)(const uint32_t);
} HANDLER_os_delay_t;
// ============================ os delay typedef =============================

typedef struct handler_os_queue_t {
    LED_HANDLER_STATUS_t    (*pf_os_queue_create)(const uint32_t item_num,
                                                  uint32_t const item_size,
                                                  void ** const que_handler);
    LED_HANDLER_STATUS_t    (*pf_os_queue_put)(void * const que_handler,
                                               void * const msg,
                                               uint32_t timeout);
    LED_HANDLER_STATUS_t    (*pf_os_queue_get)(void * const que_handler,
                                               void * const msg,
                                               uint32_t timeout);
    LED_HANDLER_STATUS_t    (*pf_os_queue_delete)(void * const que_handler);
} handler_os_queue_t;
// ============================ os queue typedef =============================

typedef struct handler_os_critical_t   {
    LED_HANDLER_STATUS_t    (*pf_os_p)(void);
    LED_HANDLER_STATUS_t    (*pf_os_v)(void);
}handler_os_critical_t;
// ========================== os critical typedef ============================

typedef struct handler_os_thread_t   {
    LED_HANDLER_STATUS_t    (*pf_os_thread_create)(
                                                    void * const task_code,
                                                    const char * const task_name,
                                                    const uint32_t stack_depth,
                                                    void * const parameters,
                                                    const uint32_t task_priority,
                                                    void ** const task_handle
                                                    );
    LED_HANDLER_STATUS_t    (*pf_os_thread_delete)(void);
}handler_os_thread_t;

#endif  //OS_SUPPORTING
// ====================== handler forward declaration ========================

typedef struct bsp_led_handler_t bsp_led_handler_t;
// ========================= handler callback typedef ========================
// ========================= handler control typedef =========================

typedef LED_HANDLER_STATUS_t (*pf_led_handler_control)(bsp_led_handler_t * const self,
                                                       LED_INDEX_T led_index,
                                                       uint32_t cycle_time_ms,
                                                       uint32_t blink_time_ms,
                                                       LED_PROPORTION_t proportion_on_off);
// ======================== handler register typedef =========================

typedef LED_HANDLER_STATUS_t (*pf_led_handler_register)(bsp_led_handler_t * const self,
                                                        bsp_led_driver_t  * const led_driver,
                                                        LED_INDEX_T       * const led_index);
// =========================== handler object type ===========================

typedef struct bsp_led_handler_t {
    LED_HANDLER_INIT_STATUS_t   is_inited;
    instance_registered_t       instances;
    time_base_ms_t              *p_time_base_ms;
    void                        *queue_handler;//'void' to fit every os
    void                        *thread_handler;//'void' to fit every os
#ifdef  OS_SUPPORTING
    os_delay_t                  *p_os_delay_ms;
    handler_os_queue_t          *p_os_queue_interface;
    handler_os_critical_t       *p_os_critical;
    handler_os_thread_t         *p_os_thread;
#endif  //OS_SUPPORTING
    pf_led_handler_control      bsp_led_handler_control;
    pf_led_handler_register     bsp_led_handler_register;
} bsp_led_handler_t;

// ================================ Define end =================================

// ================================ Declaring ===================================
LED_HANDLER_STATUS_t handler_led_inst(
    bsp_led_handler_t * const self,
    time_base_ms_t    * const time_base,
#ifdef  OS_SUPPORTING
    handler_os_critical_t * const os_critical,
    os_delay_t        * const os_delay,
    handler_os_queue_t * const os_queue,
    handler_os_thread_t * const os_thread
#endif  //OS_SUPPORTING
);
// ===================== handler register declaration ========================

LED_HANDLER_STATUS_t handler_led_register(bsp_led_handler_t * const self,
                                          bsp_led_driver_t  * const led_driver,
                                          LED_INDEX_T       * const led_index);
// ====================== handler control declaration =======================

LED_HANDLER_STATUS_t handler_led_control(bsp_led_handler_t * const self,
                                         LED_INDEX_T led_index,
                                         uint32_t cycle_time_ms,
                                         uint32_t blink_time_ms,
                                         LED_PROPORTION_t proportion_on_off);
// ================================ Declaring End ===============================

#endif
