/**
  ******************************************************************************
  * @file    STM32F0xx_IAP/inc/ymodem.h 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    29-May-2012
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _YMODEM_H_
#define _YMODEM_H_

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define PACKET_SEQNO_INDEX      (1)
#define PACKET_SEQNO_COMP_INDEX (2)

#define PACKET_HEADER           (3)
#define PACKET_TRAILER          (2)
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE             (128)
#define PACKET_1K_SIZE          (1024)

#define FILE_NAME_LENGTH        (256)
#define FILE_SIZE_LENGTH        (16)

#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define EOT                     (0x04)  /* end of transmission 结束传输 */
#define ACK                     (0x06)  /* acknowledge 确认*/
#define NAK                     (0x15)  /* negative acknowledge 无确认 */
#define CA                      (0x18)  /* two of these in succession aborts transfer 其中两个相继中止传输 */
#define CRC16                   (0x43)  /* 'C' == 0x43, request 16-bit CRC 请求CRC */

#define ABORT1                  (0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  (0x61)  /* 'a' == 0x61, abort by user */

#define NAK_TIMEOUT             (0x100000)//无确认超时时间
#define MAX_ERRORS              (3)//最大错误数量

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/**
 * @brief 使用 Ymodem 协议接收文件并写入 Flash。
 * @param buf 接收缓冲区，容量至少为 1024 字节。
 * @return 成功返回文件大小；取消或错误超限返回 0；
 *         -1：文件过大；-2：写入校验失败；-3：用户取消。
 */
int32_t Ymodem_Receive (uint8_t *buf);

/**
 * @brief 使用 Ymodem 协议发送文件。
 * @param buf 文件数据缓冲区。
 * @param sendFileName 待发送的文件名。
 * @param sizeFile 文件大小，单位为字节。
 * @return 0：成功；重试超限返回错误计数；0xFF：包数达到限制。
 */
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t *sendFileName,
                        uint32_t sizeFile);

/**
 * @brief 根据单个字节更新 CRC16。
 * @param crcIn 当前 CRC 值。
 * @param byte 输入字节。
 * @return 更新后的 CRC16 值。
 */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte);

/**
 * @brief 计算 Ymodem 数据区的 CRC16。
 * @param data 数据首地址。
 * @param size 数据长度，单位为字节。
 * @return CRC16 校验值。
 */
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size);

/**
 * @brief 计算数据的累加校验和，保留低 8 位。
 * @param data 数据首地址。
 * @param size 数据长度，单位为字节。
 * @return 8 位校验和。
 */
uint8_t CalChecksum(const uint8_t* data, uint32_t size);

/**
 * @brief 检查响应的预留接口，当前未实现检查逻辑。
 * @param c 响应字节，当前未使用。
 * @return 固定返回 0。
 */
int32_t Ymodem_CheckResponse(uint8_t c);

/**
 * @brief 构造包含文件名和文件大小的首包，不含校验字段。
 * @param data 输出缓冲区，至少容纳 PACKET_HEADER + PACKET_SIZE 字节。
 * @param fileName 以空字符结尾的文件名。
 * @param length 文件大小的地址，单位为字节，只读取该值。
 */
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName,
                              uint32_t *length);

/**
 * @brief 构造数据包，不足部分填充 0x1A，不含校验字段。
 * @param SourceBuf 源数据首地址。
 * @param data 输出缓冲区，至少容纳 PACKET_HEADER + PACKET_1K_SIZE 字节。
 * @param pktNo 数据包序号。
 * @param sizeBlk 剩余数据长度，用于选择包长和确定复制长度。
 */
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data,
                         uint8_t pktNo, uint32_t sizeBlk);

/**
 * @brief 逐字节发送数据包，不自动追加校验字段。
 * @param data 待发送数据首地址。
 * @param length 待发送字节数。
 */
void Ymodem_SendPacket(uint8_t *data, uint16_t length);

#endif  /* _YMODEM_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
