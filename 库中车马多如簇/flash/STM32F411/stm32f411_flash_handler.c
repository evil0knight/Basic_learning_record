#include "stm32f411_flash_handler.h"

#include <stddef.h>
#include <string.h>

#include "flash.h"

en_stm32f411_flash_status_t stm32f411_flash_handler_erase(uint32_t address,
                                                           uint32_t size)
{
    if (size == 0U)
    {
        return STM32F411_FLASH_ERR_ARGUMENT;
    }
    return (Flash_erase(address, size) == 0U)
               ? STM32F411_FLASH_OK : STM32F411_FLASH_ERR_ERASE;
}

en_stm32f411_flash_status_t stm32f411_flash_handler_read(uint32_t address,
                                                          uint8_t *data,
                                                          uint32_t size)
{
    if ((data == NULL) || (size == 0U))
    {
        return STM32F411_FLASH_ERR_ARGUMENT;
    }
    memcpy(data, (const void *)(uintptr_t)address, size);
    return STM32F411_FLASH_OK;
}

en_stm32f411_flash_status_t stm32f411_flash_handler_write(
    uint32_t address, const uint8_t *data, uint32_t size)
{
    uint32_t remaining = size;

    if ((data == NULL) || (size == 0U))
    {
        return STM32F411_FLASH_ERR_ARGUMENT;
    }
    while (remaining > 0U)
    {
        uint32_t word = 0xFFFFFFFFUL;
        uint32_t chunk = (remaining < sizeof(word)) ? remaining : sizeof(word);

        memcpy(&word, data, chunk);
        Flash_Write(address, word);
        if (memcmp((const void *)(uintptr_t)address, &word, chunk) != 0)
        {
            return STM32F411_FLASH_ERR_WRITE;
        }
        address += sizeof(word);
        data += chunk;
        remaining -= chunk;
    }
    return STM32F411_FLASH_OK;
}
