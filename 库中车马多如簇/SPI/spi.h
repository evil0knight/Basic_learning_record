/******************************************************************************
 * Copyright (C) 2024 XXX, XXX or its affiliates.
 *
 * @file Spi.h
 *
 * @par dependencies
 * - Spi.h
 *
 * @author XXX
 *
 * @brief Functions related to reading and writing in the chip's flash area.
 *
 * Processing flow:
 *
 * call directly.
 *
 * @version V1.0 2024-09-13
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef __SPI_H
#define __SPI_H

#include "main.h"
#include "stm32f4xx.h"

#define F_CS_Pin       GPIO_Pin_4 // TODO: 改为实际片选引脚
#define F_CS_GPIO_Port GPIOA      // TODO: 改为实际片选端口

void SPI1_Init(void);
u8 SPI1_WriteByte(u8 *WriteData, u16 dataSize, u32 timeout);
u8 SPI1_ReadByte(u8 *ReadData, u16 dataSize, u32 timeout);

#endif /* __SPI_H */
