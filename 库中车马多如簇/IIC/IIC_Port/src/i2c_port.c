#include "i2c_port.h"

#include <stddef.h>

#include "soft_i2c.h"

typedef enum
{
    CORE_I2C_TYPE_NONE = 0,
    CORE_I2C_TYPE_HARDWARE,
    CORE_I2C_TYPE_SOFTWARE
} en_core_i2c_type_t;

typedef struct
{
    en_core_i2c_type_t type;
    union
    {
        st_core_hardware_i2c_config_t hardware;
        st_core_software_i2c_config_t software;
    } config;
} st_core_i2c_slot_t;

#include "i2c_port_config.h"

static en_core_i2c_status_t core_i2c_validate_port(core_i2c_port_id_t port)
{
    const st_core_i2c_slot_t *slot;

    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    slot = &g_i2c_slots[port];
    if (slot->type == CORE_I2C_TYPE_NONE)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if ((slot->type != CORE_I2C_TYPE_HARDWARE) &&
        (slot->type != CORE_I2C_TYPE_SOFTWARE))
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if ((slot->type == CORE_I2C_TYPE_HARDWARE) &&
        ((slot->config.hardware.handle == NULL) ||
         ((slot->config.hardware.lock == NULL) !=
          (slot->config.hardware.unlock == NULL))))
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (slot->type == CORE_I2C_TYPE_SOFTWARE)
    {
        const st_core_software_i2c_config_t *config = &slot->config.software;

        if ((config->scl_port == NULL) || (config->scl_pin == 0U) ||
            (config->sda_port == NULL) || (config->sda_pin == 0U) ||
            (config->half_period_us == 0U) || (config->delay_us == NULL) ||
            ((config->lock == NULL) != (config->unlock == NULL)))
        {
            return CORE_I2C_ERR_NOT_CONFIGURED;
        }
    }

    return CORE_I2C_OK;
}

static en_core_i2c_status_t core_i2c_validate_transfer(core_i2c_port_id_t port,
                                                        uint8_t device_address_7bit,
                                                        const void *data,
                                                        uint16_t size)
{
    en_core_i2c_status_t status = core_i2c_validate_port(port);

    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if ((device_address_7bit > 0x7FU) || (data == NULL) || (size == 0U))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    return CORE_I2C_OK;
}

static en_core_i2c_status_t core_i2c_validate_address_size(
    en_core_i2c_mem_addr_size_t address_size)
{
    if ((address_size != CORE_I2C_MEM_ADDR_8BIT) &&
        (address_size != CORE_I2C_MEM_ADDR_16BIT))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    return CORE_I2C_OK;
}

static en_core_i2c_status_t core_i2c_from_hal(I2C_HandleTypeDef *handle,
                                               HAL_StatusTypeDef status)
{
    if (status == HAL_OK)
    {
        return CORE_I2C_OK;
    }
    if (status == HAL_BUSY)
    {
        return CORE_I2C_ERR_BUSY;
    }
    if (status == HAL_TIMEOUT)
    {
        return CORE_I2C_ERR_TIMEOUT;
    }
    if ((handle != NULL) && ((HAL_I2C_GetError(handle) & HAL_I2C_ERROR_AF) != 0U))
    {
        return CORE_I2C_ERR_NACK;
    }

    return CORE_I2C_ERROR;
}

static core_i2c_lock_fn_t core_i2c_get_lock(const st_core_i2c_slot_t *slot)
{
    return (slot->type == CORE_I2C_TYPE_HARDWARE)
               ? slot->config.hardware.lock
               : slot->config.software.lock;
}

static core_i2c_unlock_fn_t core_i2c_get_unlock(const st_core_i2c_slot_t *slot)
{
    return (slot->type == CORE_I2C_TYPE_HARDWARE)
               ? slot->config.hardware.unlock
               : slot->config.software.unlock;
}

static en_core_i2c_status_t core_i2c_lock(const st_core_i2c_slot_t *slot,
                                          uint32_t timeout_ms)
{
    core_i2c_lock_fn_t lock = core_i2c_get_lock(slot);

    if ((lock != NULL) && (lock(timeout_ms) != 0))
    {
        return CORE_I2C_ERR_BUSY;
    }

    return CORE_I2C_OK;
}

static void core_i2c_unlock(const st_core_i2c_slot_t *slot)
{
    core_i2c_unlock_fn_t unlock = core_i2c_get_unlock(slot);

    if (unlock != NULL)
    {
        unlock();
    }
}

en_core_i2c_status_t core_i2c_write(core_i2c_port_id_t port,
                                    uint8_t device_address_7bit,
                                    const uint8_t *data,
                                    uint16_t size,
                                    uint32_t timeout_ms)
{
    st_core_i2c_slot_t *slot;
    en_core_i2c_status_t status;

    status = core_i2c_validate_transfer(port, device_address_7bit, data, size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    slot = &g_i2c_slots[port];
    status = core_i2c_lock(slot, timeout_ms);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if (slot->type == CORE_I2C_TYPE_HARDWARE)
    {
        status = core_i2c_from_hal(
            slot->config.hardware.handle,
            HAL_I2C_Master_Transmit(slot->config.hardware.handle,
                                    (uint16_t)device_address_7bit << 1U,
                                    (uint8_t *)data,
                                    size,
                                    timeout_ms));
    }
    else
    {
        status = soft_i2c_write(&slot->config.software,
                                device_address_7bit,
                                data,
                                size);
    }

    core_i2c_unlock(slot);
    return status;
}

en_core_i2c_status_t core_i2c_read(core_i2c_port_id_t port,
                                   uint8_t device_address_7bit,
                                   uint8_t *data,
                                   uint16_t size,
                                   uint32_t timeout_ms)
{
    st_core_i2c_slot_t *slot;
    en_core_i2c_status_t status;

    status = core_i2c_validate_transfer(port, device_address_7bit, data, size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    slot = &g_i2c_slots[port];
    status = core_i2c_lock(slot, timeout_ms);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if (slot->type == CORE_I2C_TYPE_HARDWARE)
    {
        status = core_i2c_from_hal(
            slot->config.hardware.handle,
            HAL_I2C_Master_Receive(slot->config.hardware.handle,
                                   (uint16_t)device_address_7bit << 1U,
                                   data,
                                   size,
                                   timeout_ms));
    }
    else
    {
        status = soft_i2c_read(&slot->config.software,
                               device_address_7bit,
                               data,
                               size);
    }

    core_i2c_unlock(slot);
    return status;
}

en_core_i2c_status_t core_i2c_mem_write(core_i2c_port_id_t port,
                                        uint8_t device_address_7bit,
                                        uint16_t memory_address,
                                        en_core_i2c_mem_addr_size_t address_size,
                                        const uint8_t *data,
                                        uint16_t size,
                                        uint32_t timeout_ms)
{
    st_core_i2c_slot_t *slot;
    en_core_i2c_status_t status;

    status = core_i2c_validate_transfer(port, device_address_7bit, data, size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }
    status = core_i2c_validate_address_size(address_size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    slot = &g_i2c_slots[port];
    status = core_i2c_lock(slot, timeout_ms);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if (slot->type == CORE_I2C_TYPE_HARDWARE)
    {
        uint16_t hal_address_size = (address_size == CORE_I2C_MEM_ADDR_8BIT)
                                        ? I2C_MEMADD_SIZE_8BIT
                                        : I2C_MEMADD_SIZE_16BIT;
        status = core_i2c_from_hal(
            slot->config.hardware.handle,
            HAL_I2C_Mem_Write(slot->config.hardware.handle,
                              (uint16_t)device_address_7bit << 1U,
                              memory_address,
                              hal_address_size,
                              (uint8_t *)data,
                              size,
                              timeout_ms));
    }
    else
    {
        status = soft_i2c_mem_write(&slot->config.software,
                                    device_address_7bit,
                                    memory_address,
                                    address_size,
                                    data,
                                    size);
    }

    core_i2c_unlock(slot);
    return status;
}

en_core_i2c_status_t core_i2c_mem_read(core_i2c_port_id_t port,
                                       uint8_t device_address_7bit,
                                       uint16_t memory_address,
                                       en_core_i2c_mem_addr_size_t address_size,
                                       uint8_t *data,
                                       uint16_t size,
                                       uint32_t timeout_ms)
{
    st_core_i2c_slot_t *slot;
    en_core_i2c_status_t status;

    status = core_i2c_validate_transfer(port, device_address_7bit, data, size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }
    status = core_i2c_validate_address_size(address_size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    slot = &g_i2c_slots[port];
    status = core_i2c_lock(slot, timeout_ms);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if (slot->type == CORE_I2C_TYPE_HARDWARE)
    {
        uint16_t hal_address_size = (address_size == CORE_I2C_MEM_ADDR_8BIT)
                                        ? I2C_MEMADD_SIZE_8BIT
                                        : I2C_MEMADD_SIZE_16BIT;
        status = core_i2c_from_hal(
            slot->config.hardware.handle,
            HAL_I2C_Mem_Read(slot->config.hardware.handle,
                             (uint16_t)device_address_7bit << 1U,
                             memory_address,
                             hal_address_size,
                             data,
                             size,
                             timeout_ms));
    }
    else
    {
        status = soft_i2c_mem_read(&slot->config.software,
                                   device_address_7bit,
                                   memory_address,
                                   address_size,
                                   data,
                                   size);
    }

    core_i2c_unlock(slot);
    return status;
}

en_core_i2c_status_t core_i2c_is_ready(core_i2c_port_id_t port,
                                       uint8_t device_address_7bit,
                                       uint32_t trials,
                                       uint32_t timeout_ms)
{
    st_core_i2c_slot_t *slot;
    en_core_i2c_status_t status;

    status = core_i2c_validate_port(port);
    if (status != CORE_I2C_OK)
    {
        return status;
    }
    if ((device_address_7bit > 0x7FU) || (trials == 0U))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    slot = &g_i2c_slots[port];
    status = core_i2c_lock(slot, timeout_ms);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if (slot->type == CORE_I2C_TYPE_HARDWARE)
    {
        status = core_i2c_from_hal(
            slot->config.hardware.handle,
            HAL_I2C_IsDeviceReady(slot->config.hardware.handle,
                                  (uint16_t)device_address_7bit << 1U,
                                  trials,
                                  timeout_ms));
    }
    else
    {
        status = soft_i2c_is_ready(&slot->config.software,
                                   device_address_7bit,
                                   trials);
    }

    core_i2c_unlock(slot);
    return status;
}
