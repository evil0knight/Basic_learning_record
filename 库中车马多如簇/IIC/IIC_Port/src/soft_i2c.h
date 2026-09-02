#ifndef SOFT_I2C_H
#define SOFT_I2C_H

#include "i2c_port.h"

en_core_i2c_status_t soft_i2c_write(const st_core_software_i2c_config_t *config,
                                    uint8_t device_address_7bit,
                                    const uint8_t *data,
                                    uint16_t size);
en_core_i2c_status_t soft_i2c_read(const st_core_software_i2c_config_t *config,
                                   uint8_t device_address_7bit,
                                   uint8_t *data,
                                   uint16_t size);
en_core_i2c_status_t soft_i2c_mem_write(const st_core_software_i2c_config_t *config,
                                        uint8_t device_address_7bit,
                                        uint16_t memory_address,
                                        en_core_i2c_mem_addr_size_t address_size,
                                        const uint8_t *data,
                                        uint16_t size);
en_core_i2c_status_t soft_i2c_mem_read(const st_core_software_i2c_config_t *config,
                                       uint8_t device_address_7bit,
                                       uint16_t memory_address,
                                       en_core_i2c_mem_addr_size_t address_size,
                                       uint8_t *data,
                                       uint16_t size);
en_core_i2c_status_t soft_i2c_is_ready(const st_core_software_i2c_config_t *config,
                                       uint8_t device_address_7bit,
                                       uint32_t trials);

#endif
