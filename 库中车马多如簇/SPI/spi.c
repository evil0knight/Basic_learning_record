/******************************************************************************
 * Copyright (C) 2024 XXX or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file Spi.c
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
 *****************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "Spi.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
u8 SPI1_ReadWriteByte(u8 WriteData);
/* Private functions ---------------------------------------------------------*/
/* extern variables ----------------------------------------------------------*/

static void CS_IO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_OUT;  //输出
    GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_4;  // TODO: 改为实际片选引脚
    GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);  // TODO: 改为实际片选端口
}

//初始化SPI1
void SPI1_Init(void)
{
    CS_IO_Init();
    GPIO_WriteBit(F_CS_GPIO_Port,F_CS_Pin,1);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1,ENABLE); // TODO: 改为目标 SPI 外设时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);  // TODO: 改为目标 GPIO 时钟

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;  //模式需要设置为复用
    GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;  //设置为推挽输出
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;  // TODO: 改为 SCK、MISO、MOSI 引脚
    GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);  // TODO: 改为 SPI 引脚所在端口

    GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_SPI1);  // TODO: 配置 SCK 复用
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_SPI1);  // TODO: 配置 MISO 复用
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_SPI1);  // TODO: 配置 MOSI 复用

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,ENABLE);  //复位SPI1
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,DISABLE);  //停止复位SPI1

    SPI_InitTypeDef SPI_InitStructure;
    SPI_InitStructure.SPI_Direction=SPI_Direction_2Lines_FullDuplex; //SPI双向全双工
    SPI_InitStructure.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_256;  // TODO: 按设备要求修改分频
    SPI_InitStructure.SPI_CPHA=SPI_CPHA_1Edge;  // TODO: 按设备要求修改采样边沿
    SPI_InitStructure.SPI_CPOL=SPI_CPOL_Low;  // TODO: 按设备要求修改时钟极性
    SPI_InitStructure.SPI_CRCPolynomial=7;  //CRC值计算多项式
    SPI_InitStructure.SPI_DataSize=SPI_DataSize_8b;  //SPI数据帧8位
    SPI_InitStructure.SPI_FirstBit=SPI_FirstBit_MSB;  //数据传输高位在前
    SPI_InitStructure.SPI_Mode=SPI_Mode_Master;  //SPI主模式，即时钟时序是由主机SCK提供的
    SPI_InitStructure.SPI_NSS=SPI_NSS_Soft;  //NSS信号由软件管理
    SPI_Init(SPI1,&SPI_InitStructure);

    SPI_Cmd(SPI1,ENABLE);  //使能SPI时钟

    SPI1_ReadWriteByte(0xff);//启动传输
}

//SPI1速度设置函数
//SPI速度=fAPB2/分频系数
//SPI_BaudRatePrescaler_4
//SPI_BaudRatePrescaler_8
//SPI_BaudRatePrescaler_16
//SPI_BaudRatePrescaler_32
//SPI_BaudRatePrescaler_64
//SPI_BaudRatePrescaler_128
//SPI_BaudRatePrescaler_256
void SPI1_SetSpeed(u8 SPI_BaudRate_Prescaler)
{
    //#define assert_param(expr) ((void)0)

    assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRate_Prescaler)); //判断有效性
    SPI1->CR1&=0xFFC7;
    SPI1->CR1|=SPI_BaudRate_Prescaler;

    SPI_Cmd(SPI1,ENABLE);  //使能SPI1
}

//SPI1读写一个字节
//WriteData:要写入的字节
//返回值：读到的字节
u8 SPI1_ReadWriteByte(u8 WriteData)
{
    while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE)==RESET);
    SPI_I2S_SendData(SPI1,WriteData);

    while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_RXNE)==RESET);
    return SPI_I2S_ReceiveData(SPI1);
}

u8 SPI1_WriteByte(u8 *WriteData, u16 dataSize, u32 timeout)
{
    u32 time = timeout;
    u32 current_time = millis();  // TODO: 工程中实现毫秒计时函数 millis()
    u16 txsize = dataSize;
    u16 rxsize = dataSize;
    u8  *pTxBuffPtr = WriteData;
    u8  txflow = 1u;
    while((txsize > 0) || (rxsize > 0))
    {
        if((SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE) !=RESET) && (txsize > 0) && (txflow==1))
        {
            SPI_I2S_SendData(SPI1,*pTxBuffPtr);
            pTxBuffPtr += sizeof(u8);
            txsize--;
            txflow = 0;
        }
        if((SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_RXNE) !=RESET) && (rxsize > 0) && (txflow==0))
        {
            SPI_I2S_ReceiveData(SPI1);
            rxsize--;
            txflow = 1;
        }
        if ((millis() - current_time) >= time )
        {
            SPI_I2S_ClearFlag(SPI1,SPI_I2S_FLAG_OVR);
            return 0;
        }
    }
    SPI_I2S_ClearFlag(SPI1,SPI_I2S_FLAG_OVR);
    return 1;
}

u8 SPI1_ReadByte(u8 *ReadData, u16 dataSize, u32 timeout)
{
    u32 time = timeout;
    u32 current_time = millis();  // TODO: 工程中实现毫秒计时函数 millis()
    u16 txsize = dataSize;
    u16 rxsize = dataSize;
    u8  *pTxBuffPtr = ReadData;
    u8  *pRxBuffPtr = ReadData;
    u8  txflow = 1u;
    while((txsize > 0) || (rxsize > 0))
    {
        if((SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE) !=RESET) && (txsize > 0) && (txflow==1))
        {
            SPI_I2S_SendData(SPI1,*pTxBuffPtr);
            pTxBuffPtr += sizeof(u8);
            txsize--;
            txflow = 0;
        }
        if((SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_RXNE) !=RESET) && (rxsize > 0) && (txflow==0))
        {
            *pRxBuffPtr = SPI_I2S_ReceiveData(SPI1);
            pRxBuffPtr += sizeof(u8);
            rxsize--;
            txflow = 1;
        }
        if ((millis() - current_time) >= time )
        {
            return 0;
        }
    }
    return 1;
}
