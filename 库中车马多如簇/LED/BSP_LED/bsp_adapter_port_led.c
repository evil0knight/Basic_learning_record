/****************************************************************************************
 * @file bsp_adapter_port_led.c
 * @brief Mount the concrete LED BSP and OS implementation into the wrapper.
 ****************************************************************************************/
#include "bsp_adapter_port_led.h"

#include <stdio.h>

#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "main.h"

#include "bsp_led_driver.h"
#include "bsp_led_handle.h"
#include "bsp_wrapper_led.h"

#ifndef LED2_GPIO_Port
#define LED2_GPIO_Port LED_GPIO_Port
#endif
#ifndef LED2_Pin
#define LED2_Pin LED_Pin
#endif

static bsp_led_handler_t g_led_handler = {
    .is_inited = LED_HANDLER_INIT_STATUS_NO
};
static bsp_led_driver_t g_led_driver[2] = {
    { .is_inited = LED_INIT_STATUS_NO },
    { .is_inited = LED_INIT_STATUS_NO },
};
static LED_INDEX_T g_led_index = LED_INDEX_INVALID;
static TaskHandle_t g_led_task_handle = NULL;

static LED_STATUS_t led_on_1(void)
{
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    return LED_OK;
}

static LED_STATUS_t led_off_1(void)
{
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    return LED_OK;
}

static LED_STATUS_t led_on_2(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    return LED_OK;
}

static LED_STATUS_t led_off_2(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
    return LED_OK;
}

static led_operations_t g_led_operations[2] = {
    { .pf_led_on = led_on_1, .pf_led_off = led_off_1 },
    { .pf_led_on = led_on_2, .pf_led_off = led_off_2 },
};

static LED_STATUS_t led_get_timebase(uint32_t * const timestamp)
{
    if (timestamp == NULL) {
        return LED_ERROR_PARAMETER;
    }
    *timestamp = HAL_GetTick();
    return LED_OK;
}

static LED_STATUS_t led_os_delay(const uint32_t delay_ms)
{
    if (osKernelRunning == osKernelGetState()) {
        osDelay(delay_ms);
    } else {
        HAL_Delay(delay_ms);
    }
    return LED_OK;
}

static time_base_ms_t g_led_timebase = { .pf_get_timebase = led_get_timebase };
static os_delay_t g_led_delay = { .pf_os_delay_ms = led_os_delay };

static LED_HANDLER_STATUS_t led_queue_create(const uint32_t item_num,
                                             uint32_t const item_size,
                                             void ** const queue_handle)
{
    if (queue_handle == NULL) {
        return HANDLER_ERROR_PARAMETER;
    }
    *queue_handle = xQueueCreate(item_num, item_size);
    return (*queue_handle == NULL) ? HANDLER_ERROR_RESOURCE : HANDLER_OK;
}

static LED_HANDLER_STATUS_t led_queue_put(void * const queue_handle,
                                          void * const message,
                                          uint32_t timeout)
{
    if ((queue_handle == NULL) || (message == NULL) || (timeout > portMAX_DELAY)) {
        return HANDLER_ERROR_PARAMETER;
    }
    return (xQueueSend(queue_handle, message, timeout) == pdTRUE) ? HANDLER_OK : HANDLER_ERROR_RESOURCE;
}

static LED_HANDLER_STATUS_t led_queue_get(void * const queue_handle,
                                          void * const message,
                                          uint32_t timeout)
{
    if ((queue_handle == NULL) || (message == NULL) || (timeout > portMAX_DELAY)) {
        return HANDLER_ERROR_PARAMETER;
    }
    return (xQueueReceive(queue_handle, message, timeout) == pdTRUE) ? HANDLER_OK : HANDLER_ERROR_RESOURCE;
}

static LED_HANDLER_STATUS_t led_queue_delete(void * const queue_handle)
{
    if (queue_handle == NULL) {
        return HANDLER_ERROR_PARAMETER;
    }
    vQueueDelete(queue_handle);
    return HANDLER_OK;
}

static handler_os_queue_t g_led_queue = {
    .pf_os_queue_create = led_queue_create,
    .pf_os_queue_put = led_queue_put,
    .pf_os_queue_get = led_queue_get,
    .pf_os_queue_delete = led_queue_delete,
};

static LED_HANDLER_STATUS_t led_enter_critical(void)
{
    vPortEnterCritical();
    return HANDLER_OK;
}

static LED_HANDLER_STATUS_t led_exit_critical(void)
{
    vPortExitCritical();
    return HANDLER_OK;
}

static handler_os_critical_t g_led_critical = {
    .pf_os_p = led_enter_critical,
    .pf_os_v = led_exit_critical,
};

static LED_HANDLER_STATUS_t led_thread_create(void * const task_code,
                                              const char * const task_name,
                                              const uint32_t stack_depth,
                                              void * const parameters,
                                              const uint32_t task_priority,
                                              void ** const task_handle)
{
    if ((task_code == NULL) || (task_name == NULL) || (parameters == NULL) ||
        (task_handle == NULL)) {
        return HANDLER_ERROR_PARAMETER;
    }
    if (xTaskCreate((TaskFunction_t)task_code,
                    task_name,
                    stack_depth,
                    parameters,
                    (UBaseType_t)task_priority,
                    &g_led_task_handle) != pdPASS) {
        return HANDLER_ERROR_RESOURCE;
    }
    *task_handle = g_led_task_handle;
    return HANDLER_OK;
}

static LED_HANDLER_STATUS_t led_thread_delete(void)
{
    if (g_led_task_handle == NULL) {
        return HANDLER_ERROR_RESOURCE;
    }
    vTaskDelete(g_led_task_handle);
    g_led_task_handle = NULL;
    return HANDLER_OK;
}

static handler_os_thread_t g_led_thread = {
    .pf_os_thread_create = led_thread_create,
    .pf_os_thread_delete = led_thread_delete,
};

static void led_adapter_init(led_wrapper_driver_t * const self)
{
    (void)self;

    if (g_led_handler.is_inited == LED_HANDLER_INIT_STATUS_NO) {
        if (handler_led_inst(&g_led_handler,
                             &g_led_timebase,
                             &g_led_critical,
                             &g_led_delay,
                             &g_led_queue,
                             &g_led_thread) != HANDLER_OK) {
            return;
        }
    }

    if (g_led_driver[0].is_inited == LED_INIT_STATUS_NO) {
        if (driver_led_inst(&g_led_driver[0], &g_led_operations[0],
                            &g_led_timebase, &g_led_delay) != LED_OK) {
            return;
        }
        (void)handler_led_register(&g_led_handler, &g_led_driver[0], &g_led_index);
    }

    if (g_led_driver[1].is_inited == LED_INIT_STATUS_NO) {
        if (driver_led_inst(&g_led_driver[1], &g_led_operations[1],
                            &g_led_timebase, &g_led_delay) != LED_OK) {
            return;
        }
        (void)handler_led_register(&g_led_handler, &g_led_driver[1], &g_led_index);
    }
}

static void led_adapter_control(led_wrapper_driver_t * const self,
                                uint32_t led_index,
                                uint32_t cycle_time_ms,
                                uint32_t blink_time_ms,
                                led_wrapper_proportion_t proportion)
{
    LED_PROPORTION_t bsp_proportion = (LED_PROPORTION_t)proportion;
    (void)self;

    if (led_index >= MAX_INSTANCE_NUMBER) {
        return;
    }
    (void)handler_led_control(&g_led_handler,
                              (LED_INDEX_T)led_index,
                              cycle_time_ms,
                              blink_time_ms,
                              bsp_proportion);
}

void drv_adapter_led_register(void)
{
    static led_wrapper_driver_t led_driver = {
        .user_data = NULL,
        .init = led_adapter_init,
        .control = led_adapter_control,
    };

    (void)drv_wrapper_led_register(&led_driver);
}
