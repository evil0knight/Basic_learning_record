#ifndef STM32F411_FLASH_HANDLER_H
#define STM32F411_FLASH_HANDLER_H

#include <stdint.h>

typedef enum
{
    STM32F411_FLASH_OK = 0,
    STM32F411_FLASH_ERR_ARGUMENT,
    STM32F411_FLASH_ERR_ERASE,
    STM32F411_FLASH_ERR_WRITE
} en_stm32f411_flash_status_t;

en_stm32f411_flash_status_t stm32f411_flash_handler_erase(uint32_t address,
                                                           uint32_t size);
en_stm32f411_flash_status_t stm32f411_flash_handler_read(uint32_t address,
                                                          uint8_t *data,
                                                          uint32_t size);
en_stm32f411_flash_status_t stm32f411_flash_handler_write(
    uint32_t address, const uint8_t *data, uint32_t size);

#endif
