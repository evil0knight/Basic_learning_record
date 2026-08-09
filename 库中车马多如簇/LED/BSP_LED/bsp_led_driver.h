/****************************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_led_driver.h
 *
 * @par dependencies
 * - stdio.h
 * - stdint.h
 *
 * @author yan gao
 *
 * @brief Provide the HAL APIs of LEDs and corresponding operations.
 *
 * Processing flow:
 *
 * call directly.
 *
 * @version V1.0 2026-6-28
 *
 * @note 1 tab == 4 spaces!
 *
 ****************************************************************************************/
#ifndef BSP_LED_DRIVER_H
#define BSP_LED_DRIVER_H
// ================================= include ==================================
#include <stdint.h>
#include <stdio.h>
// =============================== include end ================================
// ================================== Define ==================================
#define OS_SUPPORTING                   //OS_SUPPORTING depending on OS avaliable
#define DEBUG_MODE                      //DEBUG_MODE
#define DEBUG_OUTPUT(X)        printf(X)//define debug method

typedef enum{
    LED_INIT_STATUS_NO   =   0,
    LED_INIT_STATUS_OK   =   1,
}LED_INIT_STATUS_t;                            //led init status: not inited or inited ok

typedef enum {
    PROPORTION_1_1  =   0,
    PROPORTION_1_2  =   1, 
    PROPORTION_1_3  =   2, 
    PROPORTION_CUSTOM  =   0xff,
}LED_PROPORTION_t;                          //led on/off proportion: 1:1, 1:2, 1:3, or user-defined

typedef enum {
    LED_OK              =   0,
    LED_ERROR           =   1,
    LED_ERROR_TIMEOUT   =   2,
    LED_ERROR_RESOURCE  =   3,
    LED_ERROR_PARAMETER =   4,
    LED_ERROR_NO_MEMORY =   5,
    LED_ERROR_ISR       =   6,
    LED_RECEIVED        =   0XFF,
}LED_STATUS_t;                           //led operation status codes returned to caller

typedef struct led_operations_t{
    LED_STATUS_t    (*pf_led_on)(void);
    LED_STATUS_t    (*pf_led_off)(void);
}led_operations_t;                       //core layer interface: led on/off function pointers

typedef struct time_base_ms_t{
    LED_STATUS_t    (*pf_get_timebase)(uint32_t *const);
}time_base_ms_t;                         //timebase interface: get system tick in milliseconds

#ifdef  OS_SUPPORTING
typedef struct os_delay_t{
    LED_STATUS_t    (*pf_os_delay_ms)(const uint32_t);
}os_delay_t;
#endif  //OS_SUPPORTING

typedef struct bsp_led_driver_t bsp_led_driver_t;   //forward declaration

typedef LED_STATUS_t    (*pf_led_driver_control)(bsp_led_driver_t * const self,
                                                 uint32_t,      //cycle_time_ms
                                                 uint32_t,      //blink_time_ms
                                                 LED_PROPORTION_t   //proportion_on_off
                                                 );      //led blink controller function pointer type

typedef struct bsp_led_driver_t{
    LED_INIT_STATUS_t  is_inited;           //init OK or NO
    
    uint32_t        cycle_time_ms;          //the whole times of blink
    uint32_t        blink_time_ms;          //the times of light
    LED_PROPORTION_t proportion_on_off;      //the relationship of light on or off

    //=======interface from core layer========
    led_operations_t    *p_led_ops_inst;    //led operation disk-->driver
    time_base_ms_t      *p_time_base_ms;    //led about time information
    //=======interface from os layer==========
#ifdef  OS_SUPPORTING
    os_delay_t          *p_os_delay_ms;     //os provide the delay function to led
#endif  //OS_SUPPORTING
    
    pf_led_driver_control   bsp_led_driver_control;   //api for outside to controll the led
}bsp_led_driver_t;

// ================================ Define end =================================

// ================================Declaring=====================================
LED_STATUS_t    driver_led_inst (bsp_led_driver_t * const self,
                                 led_operations_t * const led_ops,
                                 time_base_ms_t   * const time_base,
#ifdef  OS_SUPPORTING
                                 os_delay_t       * const os_delay
#endif  //OS_SUPPORTING
);
/**
 * @brief Instantiate the target of bsp_led_driver_t.
 *
 * Steps:
 *  1. Adding the Core interfaces into target of bsp_led_driver instance.
 *  2. Adding the OS interfaces into target of bsp_led_driver instance.
 *  3. Adding the timebase interfaces into target of bsp_led_driver instance.
 *
 * @param[in] p_core_intf     : Pointer to the Core interfaces.
 * @param[in] p_os_intf       : Pointer to the OS interfaces.
 * @param[in] p_timebase_intf : Pointer to the timebase interfaces.
 *
 * @return led_status_t : Status of the instantiation process (e.g., success or error).
 *
 */
LED_STATUS_t    driver_led_init (bsp_led_driver_t * const self);
// ================================Declaring End===================================






#endif
