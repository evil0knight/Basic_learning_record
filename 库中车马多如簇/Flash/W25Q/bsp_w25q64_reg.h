#ifndef BSP_W25Q64_REG_H
#define BSP_W25Q64_REG_H

/* W25Q64 命令码 */
#define W25Q64_CMD_READ_ID           (0x90U)  /* 读 ID */
#define W25Q64_CMD_READ_DATA         (0x03U)  /* 读数据 */
#define W25Q64_CMD_READ_REG          (0x05U)  /* 读状态寄存器 */
#define W25Q64_CMD_ERASE_SECTOR      (0x20U)  /* 扇区擦除(4KB) */
#define W25Q64_CMD_CHIP_SECTOR       (0x07U)  /* 全片擦除 */
#define W25Q64_CMD_WRITEON_REG       (0x06U)  /* 写使能 */
#define W25Q64_CMD_WRITEOFF_REG      (0x04U)  /* 写禁止 */
#define W25Q64_CMD_WRITE_DATA        (0x02U)  /* 页写 */
#define W25Q64_CMD_SLEEP             (0xB9U)  /* 掉电(睡眠) */
#define W25Q64_CMD_WAKEUP            (0xABU)  /* 唤醒 */

/* W25Q64 存储几何 */
#define W25Q64_SECTOR_SIZE           (4096U)       /* 扇区大小 4KB */
#define W25Q64_PAGE_SIZE             (256U)        /* 页大小 256B */
#define W25Q64_MAX_SIZE              (0x00800000U) /* 总容量 8MB */

#endif /* BSP_W25Q64_REG_H */
