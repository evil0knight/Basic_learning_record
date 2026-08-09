/****************************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_led_driver.c
 *
 * @par dependencies
 * - bsp_led_driver.h
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
// ================================= include =================================
#include "bsp_led_driver.h"
// =============================== include end ===============================

//===============================declare======================================
static LED_STATUS_t driver_led_control(
    bsp_led_driver_t * const self,
    uint32_t cycle_time,
    uint32_t blink_time,
    LED_PROPORTION_t proportion
);
//================================declare end==================================

// ================================== Define ================================== 
//********************************* Defines *********************************//


/**
 * @brief Init the target of bsp_led_driver_t.
 *
 * Steps:
 *  1. Make the target at a specific status;
 *
 * @param[in] self      : Pointer to the bsp_led_driver_t instance.
 *
 * @return led_status_t : The status of running.
 *
 */
LED_STATUS_t    driver_led_init (bsp_led_driver_t * const self){
    if(NULL == self){
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("driver_led_init:NULL parameters\r\n");
#endif
        return LED_ERROR_PARAMETER;                         //check parameter
    }

    self->p_led_ops_inst->pf_led_off();
    uint32_t timestamp = 0;
    self->p_time_base_ms->pf_get_timebase(&timestamp);
    self->p_os_delay_ms->pf_os_delay_ms(600);
    return LED_OK;
}

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
LED_STATUS_t    driver_led_inst (bsp_led_driver_t * const self,
                                 led_operations_t * const led_ops,
                                 time_base_ms_t   * const time_base,
#ifdef  OS_SUPPORTING
                                 os_delay_t       * const os_delay
#endif  //OS_SUPPORTING
){
    if(NULL == self||NULL == led_ops||NULL == time_base||NULL == os_delay){
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("LED_ERROR_PARAMETER\r\n");
#endif
        return LED_ERROR_PARAMETER;                         //check parameter
    }

    if(LED_INIT_STATUS_OK == self->is_inited){                   //check reinit
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("LED_ERROR_RESOURCE\r\n");
#endif
        return LED_ERROR_RESOURCE;     
    }

    self->p_led_ops_inst = led_ops;                         //adding interface
    self->p_time_base_ms = time_base;
#ifdef  OS_SUPPORTING
    self->p_os_delay_ms = os_delay;
#endif  //OS_SUPPORTING
    

    self->cycle_time_ms = 0;
    self->blink_time_ms = 0;
    self->proportion_on_off = PROPORTION_CUSTOM;                //init target
    self->bsp_led_driver_control = driver_led_control;

    if(LED_OK != driver_led_init(self)){                        //led driver init
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("LED_init failed\r\n");
#endif                       
        self->p_led_ops_inst = NULL;                         
        self->p_time_base_ms = NULL;
#ifdef  OS_SUPPORTING
    self->p_os_delay_ms = NULL;
#endif  //OS_SUPPORTING
        self->bsp_led_driver_control = NULL;
        return LED_ERROR;
    }          
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("LED_init finished\r\n");
#endif
    self->is_inited = LED_INIT_STATUS_OK;                        //set init done flag
    return LED_OK;
}

static LED_STATUS_t driver_led_blink (bsp_led_driver_t *self){
    uint32_t            cycle_time_local;
    uint32_t            blink_time_local;
    LED_PROPORTION_t    proportion_local;
    uint32_t            led_toggle_time;

    cycle_time_local    = self-> cycle_time_ms;
    blink_time_local    = self-> blink_time_ms;
    proportion_local    = self-> proportion_on_off;

    if(PROPORTION_1_1 == proportion_local)       led_toggle_time = blink_time_local / 2;
    else if(PROPORTION_1_2 == proportion_local)  led_toggle_time = blink_time_local / 3;
    else if(PROPORTION_1_3 == proportion_local)  led_toggle_time = blink_time_local / 4;
    else                                        return LED_ERROR_PARAMETER;

    for(uint32_t i = 0; i < cycle_time_local; i++){
        self->p_led_ops_inst->pf_led_on();
        self->p_os_delay_ms->pf_os_delay_ms(led_toggle_time);
        self->p_led_ops_inst->pf_led_off();
        self->p_os_delay_ms->pf_os_delay_ms(blink_time_local - led_toggle_time);
    }

    return LED_OK;
}
static LED_STATUS_t driver_led_control(
    bsp_led_driver_t * const self,
    uint32_t cycle_time,
    uint32_t blink_time,
    LED_PROPORTION_t proportion
)
{
    if(NULL == self||
       LED_INIT_STATUS_NO == self->is_inited){
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("LED have not init but used\r\n");
#endif
        return LED_ERROR_PARAMETER;
    }
    if(10000<cycle_time||1000<blink_time||proportion>PROPORTION_1_3){
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("parameter error");
#endif
        return LED_ERROR_PARAMETER;
    }
    self->cycle_time_ms         = cycle_time;
    self->blink_time_ms         = blink_time;
    self->proportion_on_off     = proportion;

    return driver_led_blink(self);
}      


// ================================ Define end =================================
