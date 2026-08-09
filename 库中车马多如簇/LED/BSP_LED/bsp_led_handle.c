/****************************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_led_handle.c
 *
 * @par dependencies
 * - bsp_led_handle.h
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
 *  3. led_control()       - trigger LED control through registered driver
 *
 * @version V1.0 2026-6-28
 *
 * @note 1 tab == 4 spaces!
 *
 ****************************************************************************************/
// ************************************include****************************************************
#include "bsp_led_handle.h"
// ************************************include end********************************************

// **********************************Define***********************************************************
//================================define local variables========================================
typedef struct handler_led_event_t{
    uint32_t cycle_time_ms;
    uint32_t blink_time_ms;
    LED_PROPORTION_t proportion_on_off;
    LED_INDEX_T index;
}handler_led_event_t;


//================================init queue fuction===============================================
static LED_HANDLER_STATUS_t __array_init(           
    bsp_led_driver_t *array[],
    uint32_t array_size)
{
    uint32_t i;

    if (NULL == array) {
        return HANDLER_ERROR_PARAMETER;
    }

    for (i = 0; i < array_size; i++) {
        array[i] = NULL;
    }

    return HANDLER_OK;
}

static LED_HANDLER_STATUS_t handler_led_blink(
    bsp_led_driver_t * const self)
{
    LED_STATUS_t ret;
//----------------------------check parameters---------------------------------------------------
    if ((NULL == self) || (NULL == self->bsp_led_driver_control)) {
        return HANDLER_ERROR_RESOURCE;
    }
//----------------------------check parameters end------------------------------------------------

    ret = self->bsp_led_driver_control(
        self,
        self->cycle_time_ms,
        self->blink_time_ms,
        self->proportion_on_off
    );

    if (LED_OK != ret) {
        return HANDLER_ERROR;
    }

    return HANDLER_OK;
}

//===========__event_process:send message from queue(handler_thread) to LED driver===================================
static LED_HANDLER_STATUS_t __event_process(
    bsp_led_handler_t * const self,
    handler_led_event_t msg)
{
    bsp_led_driver_t *led_driver;
//----------------------------check parameters------------------------------------------------
    if (NULL == self) {
        return HANDLER_ERROR_PARAMETER;
    }

    if ((msg.index >= MAX_INSTANCE_NUMBER) || (LED_INDEX_INVALID == msg.index)) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("HANDLER_ERROR_RESOURCE __event_process index checking\r\n");
#endif
        return HANDLER_ERROR_RESOURCE;
    }

    led_driver = self->instances.led_instance_group[msg.index];
    if ((NULL == led_driver) ||
        (LED_INIT_STATUS_NO == led_driver->is_inited) ||
        (NULL == led_driver->bsp_led_driver_control)) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("HANDLER_ERROR_RESOURCE at __event_process\r\n");
#endif
        return HANDLER_ERROR_RESOURCE;
    }

//----------------------------check parameters end------------------------------------------------

#ifdef DEBUG_MODE
    DEBUG_OUTPUT("Start Processing at __event_process\r\n");
#endif
    led_driver->cycle_time_ms = msg.cycle_time_ms;
    led_driver->blink_time_ms = msg.blink_time_ms;
    led_driver->proportion_on_off = msg.proportion_on_off;

    if (HANDLER_OK != handler_led_blink(led_driver)) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("event processed failed at __event_process\r\n");
#endif
        return HANDLER_ERROR;
    }

#ifdef DEBUG_MODE
    DEBUG_OUTPUT("event processed at __event_process\r\n");
#endif
    return HANDLER_OK;
}


//================================handler thread========================================================================
static void handler_thread(void *argument)
{
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_thread start\r\n");
#endif
//-----------------------------check parameters---------------------------------------------------
    if (NULL == argument ) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_thread :ERROR_PARAMETER\r\n");
#endif
        return;
    }

//-----------------------------check parameters end---------------------------------------------------
    LED_HANDLER_STATUS_t ret = HANDLER_OK;
    bsp_led_handler_t *self = (bsp_led_handler_t *)argument;
    handler_led_event_t  msg;

    for(;;)
    {
        ret = self->p_os_queue_interface->pf_os_queue_get(self->queue_handler, &msg, 0);
        if (HANDLER_OK != ret) {
            continue;
        }

        (void)__event_process(self, msg);
    }
}
//================================handler_inst===============================================
LED_HANDLER_STATUS_t handler_led_inst(
    bsp_led_handler_t * const self,
    time_base_ms_t    * const time_base,
#ifdef  OS_SUPPORTING
    handler_os_critical_t * const os_critical,
    os_delay_t        * const os_delay,
    handler_os_queue_t * const os_queue,
    handler_os_thread_t * const os_thread
#endif  //OS_SUPPORTING
)
{
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_inst start\r\n");
#endif
//----------------------check parameters------------------------------------------------------------
    if ((NULL == self) || (NULL == time_base)
#ifdef  OS_SUPPORTING
        || (NULL == os_critical) || (NULL == os_delay) || (NULL == os_queue) || (NULL == os_thread)
#endif  //OS_SUPPORTING
    ) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("HANDLER_ERROR_PARAMETER\r\n");
#endif
        return HANDLER_ERROR_PARAMETER;
    }

    if (LED_HANDLER_INIT_STATUS_OK == self->is_inited) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("HANDLER_ERROR_RESOURCE\r\n");
#endif
        return HANDLER_ERROR_RESOURCE;
    }
//-------------------------check parameters end------------------------------------------------
//-------------------------adding the interface---------------------------------------------------
    self->p_time_base_ms = time_base;
#ifdef  OS_SUPPORTING
    self->p_os_delay_ms = os_delay;
    self->p_os_queue_interface = os_queue;
    self->p_os_critical = os_critical;
    self->p_os_thread = os_thread;
#endif  //OS_SUPPORTING
    self->queue_handler = NULL;
    self->thread_handler = NULL;

    self->instances.led_instance_num = 0;

    self->bsp_led_handler_control = handler_led_control;
    self->bsp_led_handler_register = handler_led_register;
    if (HANDLER_OK != self->p_os_queue_interface->pf_os_queue_create(10,
                                                                      sizeof(handler_led_event_t),
                                                                      &(self->queue_handler))) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("HANDLER_ERROR_RESOURCE\r\n");
#endif
        return HANDLER_ERROR_RESOURCE;
    }

    if (HANDLER_OK != __array_init(self->instances.led_instance_group, MAX_INSTANCE_NUMBER)) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("HANDLER_ERROR_PARAMETER\r\n");
#endif
        self->p_os_queue_interface->pf_os_queue_delete(self->queue_handler);
        return HANDLER_ERROR_PARAMETER;
    }//queue init failed

    if (HANDLER_OK != self->p_os_thread->pf_os_thread_create((void *)handler_thread,
                                                             "led_handler_1",
                                                             256U,
                                                             self,
                                                             1U,
                                                             &(self->thread_handler))) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("HANDLER_ERROR_RESOURCE\r\n");
#endif
        self->p_os_queue_interface->pf_os_queue_delete(self->queue_handler);
        self->queue_handler = NULL;
        return HANDLER_ERROR_RESOURCE;
    }
//-------------------------adding the interface end------------------------------------------------

#ifdef DEBUG_MODE
    DEBUG_OUTPUT("handler_led_inst finished\r\n");
#endif
    self->is_inited = LED_HANDLER_INIT_STATUS_OK;
    return HANDLER_OK;
}

//================================handler register inst===============================================
LED_HANDLER_STATUS_t handler_led_register(
    bsp_led_handler_t * const self,
    bsp_led_driver_t  * const led_driver,
    LED_INDEX_T       * const led_index)
{
    uint32_t index;
//----------------------------check parameters---------------------------------------------------------
    if ((NULL == self) || (NULL == led_driver) || (NULL == led_index)) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_register:NULL parameters\r\n");
#endif
        return HANDLER_ERROR_PARAMETER;
    }

    if ((LED_HANDLER_INIT_STATUS_NO == self->is_inited) || (LED_INIT_STATUS_NO == led_driver->is_inited)) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_register:INIT STATUS ERROR\r\n");
#endif
        return HANDLER_ERROR_PARAMETER;
    }

    if (NULL == led_driver->bsp_led_driver_control) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_register:RESOURCE ERROR\r\n");
#endif
        return HANDLER_ERROR_RESOURCE;
    }

    for (index = 0; index < MAX_INSTANCE_NUMBER; index++) {
        if (self->instances.led_instance_group[index] == led_driver) {
#ifdef DEBUG_MODE
            DEBUG_OUTPUT("handler_led_register:registered again\r\n");
#endif
            return HANDLER_ERROR_RESOURCE;
        }
//----------------------------check parameters end-------------------------------------------------------------
        if (NULL == self->instances.led_instance_group[index]) {
#ifdef OS_SUPPORTING
            self->p_os_critical->pf_os_p();     //enter critical
#endif
            self->instances.led_instance_group[index] = led_driver;
            *led_index = (LED_INDEX_T)index;
            self->instances.led_instance_num++;
#ifdef DEBUG_MODE
            DEBUG_OUTPUT("handler_led_register finished\r\n");
#endif
#ifdef OS_SUPPORTING
            self->p_os_critical->pf_os_v();     //exit critical
#endif
            return HANDLER_OK;
        }
    }//end for

#ifdef DEBUG_MODE
    DEBUG_OUTPUT("handler_led_register:HANDLER_ERROR_NO_MEMORY\r\n");
#endif
    return HANDLER_ERROR_NO_MEMORY;
}
//================================handler control==================================================================
LED_HANDLER_STATUS_t handler_led_control(bsp_led_handler_t * const self,
                                         LED_INDEX_T led_index,
                                         uint32_t cycle_time_ms,
                                         uint32_t blink_time_ms,
                                         LED_PROPORTION_t proportion_on_off)
{
//----------------------------define local variables------------------------------------------------
    LED_HANDLER_STATUS_t status = HANDLER_OK;
//-------------------------------check parameters---------------------------------------------------
    if (NULL == self) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_control: HANDLER_ERROR_PARAMETER\r\n");
#endif
        return HANDLER_ERROR_PARAMETER;
    }

    if (LED_HANDLER_INIT_STATUS_NO == self->is_inited) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_control: handler have not init but used\r\n");
#endif
        return HANDLER_ERROR_PARAMETER;
    }

    if(10000<cycle_time_ms||1000<blink_time_ms
        ||proportion_on_off>PROPORTION_1_3){
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_control: parameter overflow\r\n");
#endif
        return HANDLER_ERROR_PARAMETER;
    }

    if ((led_index >= MAX_INSTANCE_NUMBER) || (LED_INDEX_INVALID == led_index)) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_control: index overflow\r\n");
#endif
        return HANDLER_ERROR_RESOURCE;
    }

    if (NULL == self->instances.led_instance_group[led_index]) {
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_control: target not registered\r\n");
#endif
        return HANDLER_ERROR_RESOURCE;
    }
//----------------------------check parameters end----------------------------------------------------------

//----------------------------put led event to queue----------------------------------------------
    handler_led_event_t led_event_temp={
        .index = led_index,
        .cycle_time_ms = cycle_time_ms,
        .blink_time_ms = blink_time_ms,
        .proportion_on_off = proportion_on_off
    }; 
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_control: send led event\r\n");
#endif

    status = self->p_os_queue_interface->pf_os_queue_put(self->queue_handler,&led_event_temp,0);
    if(HANDLER_OK == status){
#ifdef DEBUG_MODE
        DEBUG_OUTPUT("handler_led_control: send led event successfulily\r\n");
#endif
    }
//----------------------------put led event to queue end------------------------------------------
    return status;
}

// ================================ Define end =================================
