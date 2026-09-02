#include "ota_flash_handler.h"
#include <string.h>
#include "ota_config.h"
#include "service_externflash_manage.h"

/*
 * 外部 Flash 块管理（A/B 双块读写）
 * A/B 两块独立维护读写指针，底层统一走 service_externflash_manage 的
 * extern_flash_read/write（异步，写内部先擦后写）。
 * 块基地址：BLOCKA = OTA_AREA_A_ADDRESS，BLOCKB = OTA_AREA_B_ADDRESS。
 */

st_W25Q_Handler as_w25q_handler[2];

static uint32_t block_base_address(u8 block_index)
{
    return (block_index == BLOCKB) ? OTA_AREA_B_ADDRESS : OTA_AREA_A_ADDRESS;
}

/* 设置某块的有效数据大小（读之前调用） */
void SetBlockParmeter(u8 block_index, uint32_t app_size)
{
    as_w25q_handler[block_index].write_index = app_size;
    as_w25q_handler[block_index].write_databuf_index = app_size % BLOCK_SIZE;
    as_w25q_handler[block_index].write_sector_index = app_size / BLOCK_SIZE;
}

/* 初始化两块读写上下文 */
void W25Q64_Init(void)
{
    (void)memset(as_w25q_handler, 0, sizeof(as_w25q_handler));
}

/* 缓冲写，攒满 4096 字节写一次 */
uint8_t W25Q64_WriteData(u8 block_index, u8 *data, u32 length)
{
    uint32_t base = block_base_address(block_index);
    uint32_t addr;
    uint16_t index;
    uint32_t i;

    if (data == NULL)
    {
        return 1U;
    }

    for (i = 0U; i < length; i++)
    {
        index = as_w25q_handler[block_index].write_databuf_index;
        as_w25q_handler[block_index].databuf[index] = data[i];
        as_w25q_handler[block_index].write_databuf_index++;

        if (as_w25q_handler[block_index].write_databuf_index == BLOCK_SIZE)
        {
            as_w25q_handler[block_index].write_databuf_index = 0U;
            addr = base + (BLOCK_SIZE * as_w25q_handler[block_index].write_sector_index);
            if (extern_flash_write(addr, BLOCK_SIZE,
                                   as_w25q_handler[block_index].databuf) != Ext_Flash_OK)
            {
                return 1U;
            }
            as_w25q_handler[block_index].write_sector_index++;
            as_w25q_handler[block_index].write_index += BLOCK_SIZE;
        }
    }

    return 0U;
}

/* 冲刷最后不足 4096 字节的数据 */
uint8_t W25Q64_WriteData_End(u8 block_index)
{
    uint32_t base = block_base_address(block_index);
    uint32_t addr;

    if (as_w25q_handler[block_index].write_databuf_index != 0U)
    {
        addr = base + (BLOCK_SIZE * as_w25q_handler[block_index].write_sector_index);
        if (extern_flash_write(addr, as_w25q_handler[block_index].write_databuf_index,
                               as_w25q_handler[block_index].databuf) != Ext_Flash_OK)
        {
            return 1U;
        }
        as_w25q_handler[block_index].write_index += as_w25q_handler[block_index].write_databuf_index;
        as_w25q_handler[block_index].write_databuf_index = 0U;
    }

    return 0U;
}

/* 按扇区读：0=读到数据，1=读完，2=读出错 */
uint8_t W25Q64_ReadData(u8 block_index, u8 *data, u16 *length)
{
    uint32_t base = block_base_address(block_index);
    uint32_t addr;

    if ((data == NULL) || (length == NULL))
    {
        return 2U;
    }

    if (as_w25q_handler[block_index].write_index > as_w25q_handler[block_index].read_index)
    {
        if (as_w25q_handler[block_index].write_sector_index >
            as_w25q_handler[block_index].read_sector_index)
        {
            /* 跨扇区：读满一个扇区 */
            *length = BLOCK_SIZE;
            addr = base + (as_w25q_handler[block_index].read_sector_index * BLOCK_SIZE);
            if (extern_flash_read(addr, *length, data) != Ext_Flash_OK)
            {
                return 2U;
            }
            as_w25q_handler[block_index].read_sector_index++;
        }
        else
        {
            /* 扇区内：读剩余长度 */
            *length = (u16)(as_w25q_handler[block_index].write_index -
                            as_w25q_handler[block_index].read_index);
            addr = base + (as_w25q_handler[block_index].read_sector_index * BLOCK_SIZE);
            if (extern_flash_read(addr, *length, data) != Ext_Flash_OK)
            {
                return 2U;
            }
        }
        as_w25q_handler[block_index].read_index += *length;
        return 0U;
    }

    return 1U;
}
