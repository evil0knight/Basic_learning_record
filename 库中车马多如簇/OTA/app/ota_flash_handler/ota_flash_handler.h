#ifndef OTA_FLASH_HANDLER_H
#define OTA_FLASH_HANDLER_H

#include <stdint.h>

/* ===== 外部 Flash 块管理（A/B 双块）===== */

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef struct
{
    u8  databuf[4096];           /* 4096 字节读写缓冲 */
    u16 write_databuf_index;     /* 写缓冲偏移 */
    u32 write_index;             /* 总写入长度（也当"该块有效数据长度"） */
    u8  write_sector_index;      /* 写扇区号（4096 字节） */
    u32 read_index;              /* 总读取长度 */
    u8  read_sector_index;       /* 读扇区号 */
} st_W25Q_Handler;

#define BLOCKA       0
#define BLOCKB       1
#define BLOCK_SIZE   4096    /* 软件内部扇区大小 */

/* 设置某块的有效数据大小（读之前调用，决定能读多少） */
void SetBlockParmeter(u8 block_index, uint32_t app_size);

/* 初始化两块读写上下文 */
void W25Q64_Init(void);

/* 缓冲写，攒满 4096 字节写一次 */
uint8_t W25Q64_WriteData(u8 block_index, u8 *data, u32 length);

/* 冲刷最后不足 4096 字节的数据 */
uint8_t W25Q64_WriteData_End(u8 block_index);

/* 按扇区读：0=读到数据，1=读完，2=读出错 */
uint8_t W25Q64_ReadData(u8 block_index, u8 *data, u16 *length);

#endif /* OTA_FLASH_HANDLER_H */
