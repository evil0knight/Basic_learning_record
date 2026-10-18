/**
  ******************************************************************************
  * @file    IAP/src/ymodem.c
  * @author  MCD 应用团队
  * @version V3.3.0
  * @date    10/15/2010
  * @brief   本文件提供 Ymodem 协议相关的全部软件功能。
  *
  ******************************************************************************
  * @copy
  *
  * 本固件仅供参考，旨在向客户提供与其产品相关的编码信息，
  * 以节省开发时间。
  * 对于因本固件内容和/或客户将其中的编码信息用于其产品
  * 而引起的任何索赔，STMicroelectronics 对相关的直接、间接
  * 或后果性损害均不承担责任。
  *
  *
  * <h2><center>&copy; 版权所有 2010 STMicroelectronics</center></h2>
  */

/** @addtogroup IAP
  * @{
  */
  
/* 头文件 ------------------------------------------------------------------*/
#include "common.h"
#include "stm32f10x_flash.h"

/* 私有类型定义 -----------------------------------------------------------*/
/* 私有常量定义 ------------------------------------------------------------*/
/* 私有宏 -------------------------------------------------------------*/
/* 私有变量 ---------------------------------------------------------*/
uint8_t file_name[FILE_NAME_LENGTH];
uint32_t FlashDestination = BackAppAddress; 
uint16_t PageSize = PAGE_SIZE;
uint32_t EraseCounter = 0x0;
uint32_t NbrOfPage = 0;
FLASH_Status FLASHStatus = FLASH_COMPLETE;
uint32_t RamSource;
extern uint8_t tab_1024[1024];

/* 私有函数声明 -----------------------------------------------*/
/* 私有函数 ---------------------------------------------------------*/

/**
  * @brief  循环检测是否收到字节,如果收到就放到*c
  * @param  c: 字节数据,结果存放地址
  * @param  timeout: 超时,轮询次数
  * @retval 0: 已接收字节
  *         -1: 超时
  */
static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
  while (timeout-- > 0)
  {
    if (SerialKeyPressed(c) == 1)//这里的c是结果存放的地址,不是字符c
    {
      return 0;
    }
  }
  return -1;
}

/**
  * @brief  发送一个字节
  * @param  c: 字节数据
  * @retval 0: 已发送字节
  */
static uint32_t Send_Byte (uint8_t c)
{
  SerialPutChar(c);
  return 0;
}

/**
  * @brief  从发送端接收一个数据包
  * @param  data
  * @param  length
  * @param  timeout
  *     0: 传输结束
  *    -1: 发送端取消传输
  *    >0: 数据包长度
  * @retval 0: 正常返回
  *        -1: 超时或数据包错误
  *         1: 用户取消传输
  */
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
  uint16_t i, packet_size;
  uint8_t c;
  *length = 0;
  if (Receive_Byte(&c, timeout) != 0)
  {
    return -1;
  }
  switch (c)
  {
    case SOH:
      packet_size = PACKET_SIZE;
      break;
    case STX:
      packet_size = PACKET_1K_SIZE;
      break;
    case EOT:
      return 0;
    case CA:
      if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
      {
        *length = -1;
        return 0;
      }
      else
      {
        return -1;
      }
    case ABORT1:
    case ABORT2:
      return 1;
    default:
      return -1;
  }
  *data = c;
  for (i = 1; i < (packet_size + PACKET_OVERHEAD); i ++)
  {
    if (Receive_Byte(data + i, timeout) != 0)
    {
      return -1;
    }
  }
  if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
  {
    return -1;
  }
  *length = packet_size;
  return 0;
}

/**
 * @brief 使用 Ymodem 协议接收文件并写入 Flash。
 * @param buf 接收缓冲区，容量至少为 1024 字节。
 * @return 成功返回文件大小；取消或错误超限返回 0；
 *         -1：文件过大；-2：写入校验失败；-3：用户取消。
 */
int32_t Ymodem_Receive (uint8_t *buf)
{
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD], file_size[FILE_SIZE_LENGTH], *file_ptr, *buf_ptr;
  int32_t i, j, packet_length, session_done, file_done, packets_received, errors, session_begin, size = 0;

  /* 初始化 Flash 写入目标地址 */
  FlashDestination = BackAppAddress;

  for (session_done = 0, errors = 0, session_begin = 0; ;)//初始化变量，进入循环
  {
    for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
    {
      switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))
      {
        case 0:
          errors = 0;
          switch (packet_length)
          {
            /* 发送端取消传输 */
            case - 1:
              Send_Byte(ACK);
              return 0;
            /* 传输结束 */
            case 0:
              Send_Byte(ACK);
              file_done = 1;
              break;
            /* 普通数据包 */
            default:
              // 包序号与预期不符
              if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
              {
                Send_Byte(NAK);
              }
              else
              {
                if (packets_received == 0)// 首包，解析文件信息
                {
                  if (packet_data[PACKET_HEADER] != 0)// 文件名非空
                  {
                    for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
                    {// 提取文件名
                      file_name[i++] = *file_ptr++;
                    }
                    file_name[i++] = '\0';
                    for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
                    {// 提取文件大小字符串
                      file_size[i++] = *file_ptr++;
                    }
                    file_size[i++] = '\0';
                    Str2Int(file_size, &size);// 将文件大小转换为整数

                    /* 检查待传输固件的大小 */
                    /* 固件大小超过 Flash 容量 */
                    if (size > (FLASH_SIZE - 1))
                    {
                      /* 结束会话 */
                      Send_Byte(CA);
                      Send_Byte(CA);
                      return -1;
                    }

                    /* 擦除用于存放用户应用程序的 Flash 页 */
                    /* 计算需要擦除的页数 */
                    NbrOfPage = FLASH_PagesMask(size);

                    /* 擦除 Flash 页 */
										//擦除App
                    for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
                    {
                      FLASHStatus = FLASH_ErasePage(FlashDestination + (PageSize * EraseCounter));
                    }
                    Send_Byte(ACK);
                    Send_Byte(CRC16);
                  }
                  /* 文件名包为空，结束会话 */
                  else
                  {
                    Send_Byte(ACK);
                    file_done = 1;
                    session_done = 1;
                    break;
                  }
                }
                /* 数据包 */
                else
                {
                  memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);
                  RamSource = (uint32_t)buf;
                  for (j = 0;(j < packet_length) && (FlashDestination <  BackAppAddress + size);j += 4)
                  {
                    /* 将接收到的数据写入 STM32F10x 的 Flash */
                    FLASH_ProgramWord(FlashDestination, *(uint32_t*)RamSource);

                    if (*(uint32_t*)FlashDestination != *(uint32_t*)RamSource)
                    {
                      /* 结束会话 */
                      Send_Byte(CA);
                      Send_Byte(CA);
                      return -2;
                    }
                    FlashDestination += 4;
                    RamSource += 4;
                  }
                  Send_Byte(ACK);
                }
                packets_received ++;
                session_begin = 1;
              }
          }
          break;
        case 1:
          Send_Byte(CA);
          Send_Byte(CA);
          return -3;
        default:
//          if (session_begin > 0)
//          {
            errors ++;
//          }
          if (errors > MAX_ERRORS)
          {
            Send_Byte(CA);
            Send_Byte(CA);
            return 0;
          }
          Send_Byte(CRC16);
          break;
      }
      if (file_done != 0)
      {
        break;
      }
    }
    if (session_done != 0)
    {
      break;
    }
  }
  return (int32_t)size;
}

/**
 * @brief 检查响应的预留接口，当前未实现检查逻辑。
 * @param c 响应字节，当前未使用。
 * @return 固定返回 0。
 */
int32_t Ymodem_CheckResponse(uint8_t c)
{
  return 0;
}

/**
 * @brief 构造包含文件名和文件大小的首包，不含校验字段。
 * @param data 输出缓冲区，至少容纳 PACKET_HEADER + PACKET_SIZE 字节。
 * @param fileName 以空字符结尾的文件名。
 * @param length 文件大小的地址，单位为字节，只读取该值。
 */
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
  uint16_t i, j;
  uint8_t file_ptr[10];
  
  /* 设置数据包的前三个字节 */
  data[0] = SOH;
  data[1] = 0x00;
  data[2] = 0xff;
  
  /* 文件名包包含有效数据 */
  for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
  {
     data[i + PACKET_HEADER] = fileName[i];
  }

  data[i + PACKET_HEADER] = 0x00;
  
  Int2Str (file_ptr, *length);
  for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
  {
     data[i++] = file_ptr[j++];
  }
  
  for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
  {
    data[j] = 0;
  }
}

/**
 * @brief 构造数据包，不足部分填充 0x1A，不含校验字段。
 * @param SourceBuf 源数据首地址。
 * @param data 输出缓冲区，至少容纳 PACKET_HEADER + PACKET_1K_SIZE 字节。
 * @param pktNo 数据包序号。
 * @param sizeBlk 剩余数据长度，用于选择包长和确定复制长度。
 */
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
  uint16_t i, size, packetSize;
  uint8_t* file_ptr;
  
  /* 设置数据包的前三个字节 */
  packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
  size = sizeBlk < packetSize ? sizeBlk :packetSize;
  if (packetSize == PACKET_1K_SIZE)
  {
     data[0] = STX;
  }
  else
  {
     data[0] = SOH;
  }
  data[1] = pktNo;
  data[2] = (~pktNo);
  file_ptr = SourceBuf;
  
  /* 文件名包包含有效数据 */
  for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
  {
     data[i] = *file_ptr++;
  }
  if ( size  <= packetSize)
  {
    for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
    {
      data[i] = 0x1A; /* 文件结束符（0x1A）或 0x00 */
    }
  }
}

/**
 * @brief 根据单个字节更新 CRC16。
 * @param crcIn 当前 CRC 值。
 * @param byte 输入字节。
 * @return 更新后的 CRC16 值。
 */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
 uint32_t crc = crcIn;
 uint32_t in = byte|0x100;
 do
 {
 crc <<= 1;
 in <<= 1;
 if(in&0x100)
 ++crc;
 if(crc&0x10000)
 crc ^= 0x1021;
 }
 while(!(in&0x10000));
 return crc&0xffffu;
}


/**
 * @brief 计算 Ymodem 数据区的 CRC16。
 * @param data 数据首地址。
 * @param size 数据长度，单位为字节。
 * @return CRC16 校验值。
 */
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
 uint32_t crc = 0;
 const uint8_t* dataEnd = data+size;
 while(data<dataEnd)
  crc = UpdateCRC16(crc,*data++);
 
 crc = UpdateCRC16(crc,0);
 crc = UpdateCRC16(crc,0);
 return crc&0xffffu;
}

/**
 * @brief 计算数据的累加校验和，保留低 8 位。
 * @param data 数据首地址。
 * @param size 数据长度，单位为字节。
 * @return 8 位校验和。
 */
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
 uint32_t sum = 0;
 const uint8_t* dataEnd = data+size;
 while(data < dataEnd )
   sum += *data++;
 return sum&0xffu;
}

/**
 * @brief 逐字节发送数据包，不自动追加校验字段。
 * @param data 待发送数据首地址。
 * @param length 待发送字节数。
 */
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
  uint16_t i;
  i = 0;
  while (i < length)
  {
    Send_Byte(data[i]);
    i++;
  }
}

/**
 * @brief 使用 Ymodem 协议发送文件。
 * @param buf 文件数据缓冲区。
 * @param sendFileName 待发送的文件名。
 * @param sizeFile 文件大小，单位为字节。
 * @return 0：成功；重试超限返回错误计数；0xFF：包数达到限制。
 */
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{
  
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
  uint8_t FileName[FILE_NAME_LENGTH];
  uint8_t *buf_ptr, tempCheckSum ;
  uint16_t tempCRC, blkNumber;
  uint8_t receivedC[2], CRC16_F = 0, i;
  uint32_t errors, ackReceived, size = 0, pktSize;

  errors = 0;
  ackReceived = 0;
  for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
  {
    FileName[i] = sendFileName[i];
  }
  CRC16_F = 1;       
    
  /* 准备首个数据块 */
  Ymodem_PrepareIntialPacket(&packet_data[0], FileName, &sizeFile);
  
  do 
  {
    /* 发送数据包 */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
    /* 根据 CRC16_F 发送 CRC 校验值或校验和 */
    if (CRC16_F)
    {
       tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCRC >> 8);
       Send_Byte(tempCRC & 0xFF);
    }
    else
    {
       tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCheckSum);
    }
  
    /* 等待 ACK 和字符 C */
    if (Receive_Byte(&receivedC[0], 10000) == 0)  
    {
      if (receivedC[0] == ACK)
      { 
        /* 数据包传输成功 */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
  }while (!ackReceived && (errors < 0x0A));
  
  if (errors >=  0x0A)
  {
    return errors;
  }
  buf_ptr = buf;
  size = sizeFile;
  blkNumber = 0x01;
  /* 此处使用 1024 字节数据包发送数据 */
  
  
  /* 收到 NAK 时重发数据包，重试达到 10 次后结束通信 */
  while (size)
  {
    /* 准备下一个数据包 */
    Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
    ackReceived = 0;
    receivedC[0]= 0;
    errors = 0;
    do
    {
      /* 发送下一个数据包 */
      if (size >= PACKET_1K_SIZE)
      {
        pktSize = PACKET_1K_SIZE;
       
      }
      else
      {
        pktSize = PACKET_SIZE;
      }
      Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
      /* 根据 CRC16_F 发送 CRC 校验值或校验和 */
      /* 根据 CRC16_F 发送 CRC 校验值或校验和 */
      if (CRC16_F)
      {
         tempCRC = Cal_CRC16(&packet_data[3], pktSize);
         Send_Byte(tempCRC >> 8);
         Send_Byte(tempCRC & 0xFF);
      }
      else
      {
        tempCheckSum = CalChecksum (&packet_data[3], pktSize);
        Send_Byte(tempCheckSum);
      }
      
      /* 等待 ACK */
      if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
      {
        ackReceived = 1;  
        if (size > pktSize)
        {
           buf_ptr += pktSize;  
           size -= pktSize;
           if (blkNumber == (FLASH_IMAGE_SIZE/1024))
           {
             return 0xFF; /*  错误 */
           }
           else
           {
              blkNumber++;
           }
        }
        else
        {
          buf_ptr += pktSize;
          size = 0;
        }
      }
      else
      {
        errors++;
      }
    }while(!ackReceived && (errors < 0x0A));
    /* 收到 NAK 时重发数据包，重试达到 10 次后结束通信 */
    
    if (errors >=  0x0A)
    {
      return errors;
    }
    
  }
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;
  do 
  {
    Send_Byte(EOT);
    /* 发送传输结束标志 EOT */
    /* 等待 ACK */
      if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
      {
        ackReceived = 1;  
      }
      else
      {
        errors++;
      }
  }while (!ackReceived && (errors < 0x0A));
    
  if (errors >=  0x0A)
  {
    return errors;
  }
  
  /* 准备最后一个数据包 */
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;

  packet_data[0] = SOH;
  packet_data[1] = 0;
  packet_data [2] = 0xFF;

  for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
  {
     packet_data [i] = 0x00;
  }
  
  do 
  {
    /* 发送数据包 */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
    /* 根据 CRC16_F 发送 CRC 校验值或校验和 */
    tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
    Send_Byte(tempCRC >> 8);
    Send_Byte(tempCRC & 0xFF);
  
    /* 等待 ACK 和字符 C */
    if (Receive_Byte(&receivedC[0], 10000) == 0)  
    {
      if (receivedC[0] == ACK)
      { 
        /* 数据包传输成功 */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
 
  }while (!ackReceived && (errors < 0x0A));
  /* 收到 NAK 时重发数据包，重试达到 10 次后结束通信 */
  if (errors >=  0x0A)
  {
    return errors;
  }  
  
  do 
  {
    Send_Byte(EOT);
    /* 发送传输结束标志 EOT */
    /* 等待 ACK */
      if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
      {
        ackReceived = 1;  
      }
      else
      {
        errors++;
      }
  }while (!ackReceived && (errors < 0x0A));
    
  if (errors >=  0x0A)
  {
    return errors;
  }
  return 0; /* 文件发送成功 */
}

/**
  * @}
  */

/*******************(C)版权所有 2010 STMicroelectronics *****文件结束****/
