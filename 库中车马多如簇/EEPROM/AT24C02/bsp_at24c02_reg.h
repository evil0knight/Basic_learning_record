#ifndef BSP_AT24C02_REG_H
#define BSP_AT24C02_REG_H

/*
 * AT24C02 器件几何与轮询参数
 * 命令通过 I2C 器件地址 + 存储地址完成，无独立命令码。
 */

/* 器件几何 */
#define AT24C02_CAPACITY_BYTES    256U   /* 总容量 256B */
#define AT24C02_PAGE_SIZE         8U     /* 页大小 8B */
#define AT24C02_BASE_ADDRESS_7BIT 0x50U  /* 器件地址高 5 位固定 1010 */
#define AT24C02_ADDRESS_PINS_MAX  7U     /* A2:A0 取值范围 0~7 */

/* 写周期 ACK 轮询参数 */
#define AT24C02_READY_TRIALS      100U   /* 最大轮询次数 */
#define AT24C02_I2C_TIMEOUT_MS    10U    /* 单次 I2C 传输超时（毫秒） */

#endif /* BSP_AT24C02_REG_H */
