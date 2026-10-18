/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file Boot_Manager.c
 *
 * @par dependencies
 * - Boot_Manager.h
 * - service_ota_manager.h
 * - service_externflash_manage.h
 * - ota_config.h
 * - ymodem.h
 * - AES.h
 * - md5.h
 * - string.h
 *
 * @author YA
 *
 * @brief Bootloader OTA 状态机和镜像安装模板。
 *
 * Processing flow:
 *
 * 读取升级标志 -> 读取外部 Flash 镜像 -> 解密、校验 -> 写入 App 区。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "Boot_Manager.h"

#include "service_ota_manager.h"
#include "service_externflash_manage.h"
#include "ota_config.h"
#include "ymodem.h"
#include "AES.h"
#include "md5.h"
#include <string.h>


#ifndef __WEAK
#if defined(__GNUC__)
#define __WEAK __attribute__((weak))
#else
#define __WEAK
#endif
#endif

#define BOOT_BLOCK_A (0U)                 /* 外部 Flash A 区索引。 */
#define BOOT_BLOCK_B (1U)                 /* 外部 Flash B 区索引。 */
#define BOOT_READ_BUFFER_SIZE (4096U)     /* 镜像搬运的 RAM 缓存大小。 */
#define BOOT_FORCE_JUMP_MAGIC (0x55AA55AAUL) /* 强制跳转标志值。 */
#define BOOT_FORCE_JUMP_CLEAR (0xFFFFFFFFUL) /* 强制跳转标志清除值。 */
#define BOOT_VTOR_REGISTER (0xE000ED08UL) /* Cortex-M 向量表寄存器地址。 */

static uint8_t s_read_buffer[BOOT_READ_BUFFER_SIZE]; /* 镜像读取缓存。 */
static uint8_t s_download_buffer[BUFFER_SIZE];       /* Ymodem 接收缓存。 */
static uint8_t s_aes_key[OTA_AES_KEY_SIZE] = OTA_AES_KEY_BYTES; /* AES 密钥。 */
static uint8_t s_aes_iv[OTA_AES_BLOCK_SIZE] = OTA_AES_IV_BYTES; /* AES 初始向量。 */
static st_App_Info_t s_boot_app_info;                /* App 升级标志缓存。 */

/* 复位后保留的强制跳转标志，产品工程应放入不清零的 SRAM 区域。 */
__WEAK uint32_t g_JumpInit;


/**
 * @brief 请求 Bootloader 复位。
 * @return void : 无返回值。
 */
__WEAK void boot_system_reset(void)
{
}


/**
 * @brief 反初始化 Bootloader 使用的时钟和外设。
 * @return void : 无返回值。
 */
__WEAK void boot_clock_deinit(void)
{
}


/**
 * @brief 跳转到 App。
 * @param[in] address : App 向量表地址。
 * @return void : 无返回值。
 */
__WEAK void boot_jump_to_app(uint32_t address)
{
    uint32_t stack_top;
    uint32_t reset_handler;
    pFunction app_entry;

    /* 地址为空时不执行跳转，避免访问无效的向量表。 */
    if (0U == address)
    {
        return;
    }

    /* 向量表第一个字是栈顶，第二个字是 App 的复位入口。 */
    stack_top = *((volatile uint32_t *)(uintptr_t)address);
    reset_handler = *((volatile uint32_t *)(uintptr_t)
                     (address + sizeof(uint32_t)));

    /* 再次检查栈顶，防止挂载层误传非法 App 地址。 */
    if (((stack_top & 0x2FFE0000UL) != 0x20000000UL) ||
        (0U == reset_handler))
    {
        return;
    }

#if defined(__GNUC__) && (defined(__arm__) || defined(__thumb__))
    /* 关闭中断，防止 Bootloader 的中断干扰 App 启动。 */
    __asm volatile ("cpsid i" ::: "memory");

    /* 释放 Bootloader 的时钟和外设资源，避免影响 App 初始化。 */
    boot_clock_deinit();

    /* 将中断向量表切换到 App 的向量表。 */
    *((volatile uint32_t *)(uintptr_t)BOOT_VTOR_REGISTER) = address;

    /* 确保向量表写入在继续执行前对处理器可见。 */
    __asm volatile ("dsb\n"
                   "isb\n"
                   ::: "memory");

    /* 使用 App 的初始栈顶，切换到 App 自己的运行栈。 */
    __asm volatile ("msr msp, %0" :: "r"(stack_top) : "memory");

    /* 从向量表第二个字取得 Reset_Handler 并进入 App。 */
    app_entry = (pFunction)reset_handler;
    app_entry();
#else
    /* 非 ARM 模板构建不能执行真实跳转，由产品工程覆盖本函数。 */
    (void)app_entry;
    (void)stack_top;
    (void)reset_handler;
#endif
}


/**
 * @brief 初始化升级校验环境。
 * @return int8_t : 0 表示成功，非 0 表示失败。
 */
__WEAK int8_t boot_aes_init(void)
{
    return 0;
}


/**
 * @brief 擦除片内 Flash 应用区域。
 * @param[in] address : 擦除起始地址。
 * @param[in] size : 擦除长度，单位为字节。
 * @return void : 无返回值。
 */
__WEAK void Flash_erase(uint32_t address, uint32_t size)
{
    (void)address;
    (void)size;
}


/**
 * @brief 向片内 Flash 写入应用数据。
 * @param[in] address : 写入起始地址。
 * @param[in] data : 待读写入的数据。
 * @param[in] size : 写入长度，单位为字节。
 * @return void : 无返回值。
 */
__WEAK void Flash_Write(uint32_t address,
                             const uint8_t *data,
                             uint32_t size)
{
    (void)address;
    (void)data;
    (void)size;
}


/**
 * @brief 从片内 Flash 读取应用数据。
 * @param[in] address : 读取起始地址。
 * @param[out] data : 读取数据的缓冲区。
 * @param[in] size : 读取长度，单位为字节。
 * @return void : 无返回值。
 */
__WEAK void Flash_Read(uint32_t address, uint8_t *data, uint32_t size)
{
    (void)address;
    (void)data;
    (void)size;
}


/**
 * @brief 写入一个应用状态标志及其反码。
 * @param[in] type : 应用标志类型。
 * @param[in] value : 应用标志值。
 * @return void : 无返回值。
 */
static void SetAppFlagValue(en_App_FlagType_t type,
                                    uint32_t value)
{
    /* 根据标志类型更新对应字段。 */
    switch (type)
    {
        case App_RunState:
            s_boot_app_info.u32_App_RunState = value;
            s_boot_app_info.u32_App_RunState_Anti = ~value;
            break;
        case App_AreaASize:
            s_boot_app_info.u32_App_AreaASize = value;
            s_boot_app_info.u32_App_AreaASize_Anti = ~value;
            break;
        case App_AreaBState:
            s_boot_app_info.u32_App_AreaBState = value;
            s_boot_app_info.u32_App_AreaBState_Anti = ~value;
            break;
        case App_AreaBSize:
            s_boot_app_info.u32_App_AreaBSize = value;
            s_boot_app_info.u32_App_AreaBSize_Anti = ~value;
            break;
        default:
            return;
    }

    /* 擦除旧标志后，写入新的标志结构。 */
    Flash_erase(AppFlagAddress, sizeof(s_boot_app_info));
    Flash_Write(AppFlagAddress,
                     (const uint8_t *)&s_boot_app_info,
                     sizeof(s_boot_app_info));
}


/**
 * @brief 读取并校验一个应用状态标志。
 * @param[in] type : 应用标志类型。
 * @param[out] value : 返回应用标志值。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
static uint8_t ReadAppFlagValue(en_App_FlagType_t type,
                                        uint32_t *value)
{
    if (NULL == value)
    {
        return 1U;
    }

    /* 读取片内 Flash 中完整的标志结构。 */
    Flash_Read(AppFlagAddress,
                    (uint8_t *)&s_boot_app_info,
                    sizeof(s_boot_app_info));

    /* 通过值和反码关系校验指定字段。 */
    switch (type)
    {
        case App_RunState:
            if (s_boot_app_info.u32_App_RunState ==
                ~s_boot_app_info.u32_App_RunState_Anti)
            {
                *value = s_boot_app_info.u32_App_RunState;
                return 0U;
            }
            break;
        case App_AreaASize:
            if (s_boot_app_info.u32_App_AreaASize ==
                ~s_boot_app_info.u32_App_AreaASize_Anti)
            {
                *value = s_boot_app_info.u32_App_AreaASize;
                return 0U;
            }
            break;
        case App_AreaBState:
            if (s_boot_app_info.u32_App_AreaBState ==
                ~s_boot_app_info.u32_App_AreaBState_Anti)
            {
                *value = s_boot_app_info.u32_App_AreaBState;
                return 0U;
            }
            break;
        case App_AreaBSize:
            if (s_boot_app_info.u32_App_AreaBSize ==
                ~s_boot_app_info.u32_App_AreaBSize_Anti)
            {
                *value = s_boot_app_info.u32_App_AreaBSize;
                return 0U;
            }
            break;
        default:
            break;
    }

    return 1U;
}


/**
 * @brief 检查应用标志并跳转到 App。
 * @return void : 无返回值。
 */
void BootManager_JumpToApp(void)
{
    uint32_t state = 0U;
    uint32_t stack_top = 0U;
    uint8_t force_jump = 0U;

    /* 检查是否存在复位后强制跳转请求。 */
    if (BOOT_FORCE_JUMP_MAGIC == g_JumpInit)
    {
        g_JumpInit = BOOT_FORCE_JUMP_CLEAR;
        force_jump = 1U;
    }

    /* 普通跳转必须先确认 App 运行标志有效。 */
    if ((0U == force_jump) &&
        ((ReadAppFlagValue(App_RunState, &state) != 0U) ||
         (APP_Valid != state)))
    {
        return;
    }

    /* 读取 App 向量表中的初始栈指针。 */
    Flash_Read(ApplicationAddress,
               (uint8_t *)&stack_top,
               sizeof(stack_top));

    /* 栈顶地址无效时禁止跳转，避免执行非法地址。 */
    if ((stack_top & 0x2FFE0000UL) != 0x20000000UL)
    {
        return;
    }

    /* 由芯片工程完成中断关闭、VTOR、MSP 和复位向量跳转。 */
    boot_jump_to_app(ApplicationAddress);
}


/**
 * @brief 检查镜像长度和 App 栈顶地址。
 * @param[in] file_size : 镜像长度，单位为字节。
 * @return int8_t : 0 表示有效，-1 表示无效。
 */
int8_t CheckAppValid(int32_t file_size)
{
    uint32_t stack_top = 0U;

    /* 过滤空镜像和超过 App 区容量的镜像。 */
    if ((file_size <= 0) || ((uint32_t)file_size > MAXAPP_SIZE))
    {
        return -1;
    }

    /* App 向量表第一个字为初始栈指针。 */
    Flash_Read(ApplicationAddress,
                    (uint8_t *)&stack_top,
                    sizeof(stack_top));

    /* 栈顶地址必须位于片内 SRAM 地址范围。 */
    return ((stack_top & 0x2FFE0000UL) == 0x20000000UL) ? 0 : -1;
}


/**
 * @brief 初始化升级校验使用的 AES 环境。
 * @return int8_t : 0 表示成功，非 0 表示失败。
 */
int8_t InitAESKeyAndIV(void)
{
    return boot_aes_init();
}


/**
 * @brief 根据分区索引取得外部 Flash 起始地址。
 * @param[in] block_index : A 区或 B 区索引。
 * @param[out] address : 返回分区起始地址。
 * @return int8_t : 0 表示成功，-1 表示索引无效。
 */
static int8_t boot_get_block_address(uint8_t block_index, uint32_t *address)
{
    if (NULL == address)
    {
        return -1;
    }

    if (BOOT_BLOCK_A == block_index)
    {
        *address = OTA_AREA_A_ADDRESS;
    }
    else if (BOOT_BLOCK_B == block_index)
    {
        *address = OTA_AREA_B_ADDRESS;
    }
    else
    {
        return -1;
    }

    return 0;
}


/**
 * @brief 从外部 Flash 分区读取一个数据块。
 * @param[in] block_address : 分区起始地址。
 * @param[in] offset : 分区内偏移地址。
 * @param[out] data : 读取数据的缓冲区。
 * @param[in] size : 读取长度，单位为字节。
 * @return int8_t : 0 表示成功，-1 表示失败。
 */
static int8_t boot_read_external(uint32_t block_address,
                                 uint32_t offset,
                                 uint8_t *data,
                                 uint32_t size)
{
    if ((NULL == data) || (0U == size))
    {
        return -1;
    }

    return (extern_flash_read(block_address + offset, size, data) ==
            Ext_Flash_OK) ? 0 : -1;
}


/**
 * @brief 从外部 Flash 读取、解密并写入 App 区。
 * @param[in] block_index : 外部 Flash 分区索引。
 * @return int8_t : 0 表示成功，1 表示失败。
 */
int8_t BootManager_BackToApp(uint8_t block_index)
{
    uint8_t encrypted_block[OTA_AES_BLOCK_SIZE];
    uint8_t expected_md5[OTA_AES_BLOCK_SIZE] = {0U};
    uint32_t block_address = 0U;
    uint32_t source_offset = 0U;
    uint32_t buffer_index = 0U;
    uint32_t buffer_size = 0U;
    uint32_t app_size = 0U;
    uint32_t app_data_size;
    uint32_t processed = 0U;
    uint32_t app_address = ApplicationAddress;
    uint32_t block_count;
    uint32_t block_counter;
    uint32_t md5_collected = 0U;
    MD5_CTX md5_context;

    /* 根据 A/B 分区索引取得外部 Flash 地址。 */
    if (boot_get_block_address(block_index, &block_address) != 0)
    {
        return 1;
    }

    /* 读取并解密首个 16 字节，得到镜像总长度。 */
    if (boot_read_external(block_address,
                           source_offset,
                           s_read_buffer,
                           BOOT_READ_BUFFER_SIZE) != 0)
    {
        return 1;
    }
    buffer_size = BOOT_READ_BUFFER_SIZE;
    buffer_index = 0U;

    /* 复制镜像头并解密，头部保存镜像总长度。 */
    memcpy(encrypted_block, s_read_buffer, sizeof(encrypted_block));
    Aes_IV_key256bit_Decode(s_aes_iv, encrypted_block, s_aes_key);

    /* 按大端格式解析镜像长度字段。 */
    app_size = ((uint32_t)encrypted_block[15] << 24U) |
               ((uint32_t)encrypted_block[14] << 16U) |
               ((uint32_t)encrypted_block[13] << 8U) |
               (uint32_t)encrypted_block[12];

    /* 镜像总长度包含最后 16 字节 MD5。 */
    if ((app_size < OTA_AES_BLOCK_SIZE) ||
        (app_size > (MAXAPP_SIZE + OTA_AES_BLOCK_SIZE)))
    {
        return 1;
    }

    /* 去掉末尾 MD5 后，得到实际 App 数据长度。 */
    app_data_size = app_size - OTA_AES_BLOCK_SIZE;

    /* 计算需要解密处理的 AES 数据块数量。 */
    block_count = (app_size + OTA_AES_BLOCK_SIZE - 1U) /
                  OTA_AES_BLOCK_SIZE;
    /* 首个 16 字节已经被头部解析消耗。 */
    source_offset = 0U;
    buffer_index = OTA_AES_BLOCK_SIZE;

    /* 擦除 App 区，准备写入解密后的应用数据。 */
    Flash_erase(ApplicationAddress, app_data_size);

    /* 初始化 MD5 上下文，后续只校验 App 数据。 */
    MD5Init(&md5_context);

    for (block_counter = 0U;
         block_counter < block_count;
         ++block_counter)
    {
        /* 检查当前 4 KB RAM 缓存是否还剩完整的 AES 数据块。 */
        if ((buffer_size - buffer_index) < OTA_AES_BLOCK_SIZE)
        {
            /* 已消费的缓存长度就是下一次 Flash 读取的偏移量。 */
            source_offset += buffer_index;

            /* 当前缓存不足 16 字节时，重新读取下一个 4 KB 数据块。 */
            if (boot_read_external(block_address,
                                   source_offset,
                                   s_read_buffer,
                                   BOOT_READ_BUFFER_SIZE) != 0)
            {
                /* 外部 Flash 读取失败，终止本次镜像搬运。 */
                return 1;
            }

            /* 新缓存已经装满，读取索引从缓存首地址重新开始。 */
            buffer_size = BOOT_READ_BUFFER_SIZE;
            buffer_index = 0U;
        }

        /* 从 RAM 缓存取出一个 16 字节密文块。 */
        memcpy(encrypted_block,
               &s_read_buffer[buffer_index],
               OTA_AES_BLOCK_SIZE);

        /* 消费当前 AES 块，并解密得到明文数据。 */
        buffer_index += OTA_AES_BLOCK_SIZE;
        Aes_IV_key256bit_Decode(s_aes_iv, encrypted_block, s_aes_key);

        /* 只把应用数据送入 MD5 和片内 Flash。 */
        {
            uint32_t remain = (app_data_size > processed) ?
                              (app_data_size - processed) : 0U;
            uint32_t write_size = (remain > OTA_AES_BLOCK_SIZE) ?
                                  OTA_AES_BLOCK_SIZE : remain;

            if (write_size > 0U)
            {
                /* 计算当前明文块的摘要。 */
                MD5Update(&md5_context, encrypted_block, write_size);

                /* 将有效 App 数据写入片内 Flash。 */
                Flash_Write(app_address,
                            encrypted_block,
                            write_size);
                app_address += write_size;
                processed += write_size;
            }

            /* 应用数据不足一个 AES 块时，剩余部分属于 MD5。 */
            if ((remain < OTA_AES_BLOCK_SIZE) &&
                (md5_collected < OTA_AES_BLOCK_SIZE))
            {
                uint32_t copy_size = OTA_AES_BLOCK_SIZE - write_size;
                if (copy_size > (OTA_AES_BLOCK_SIZE - md5_collected))
                {
                    copy_size = OTA_AES_BLOCK_SIZE - md5_collected;
                }

                /* 保存镜像末尾携带的期望 MD5 值。 */
                memcpy(&expected_md5[md5_collected],
                       &encrypted_block[write_size],
                       copy_size);
                md5_collected += copy_size;
            }
        }
    }

    /* 完成 MD5 计算并比较镜像尾部摘要。 */
    {
        uint8_t computed_md5[OTA_AES_BLOCK_SIZE];

        /* 完成实际 App 数据的 MD5 计算。 */
        MD5Final(&md5_context, computed_md5);

        /* 检查 MD5 长度和摘要内容是否一致。 */
        if ((md5_collected != OTA_AES_BLOCK_SIZE) ||
            (memcmp(computed_md5,
                    expected_md5,
                    OTA_AES_BLOCK_SIZE) != 0))
        {
            return 1;
        }
    }

    /* 最后检查 App 向量表栈顶地址是否有效。 */
    return (CheckAppValid((int32_t)app_data_size) == 0) ? 0 : 1;
}


/**
 * @brief 通过 Ymodem 接收并保存镜像。
 * 这里用在AB区,运行区文件全部无效的时候,兜底使用
 * @return void : 无返回值。
 */
void BootManager_BootDownload(void)
{
    int32_t file_size;

    /* Ymodem 负责接收镜像，具体存储挂载由产品工程提供。 */
    file_size = Ymodem_Receive(s_download_buffer);
    if (file_size > 0)
    {
        SetAppFlagValue(App_AreaASize, (uint32_t)file_size);
        SetAppFlagValue(App_RunState, APP_WAIT_UPDATE);
        boot_system_reset();
    }
}


/**
 * @brief 将 A 区镜像复制到 B 区备份区。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
uint8_t Boot_Manager_AareaToBarea(void)
{
    uint32_t image_size = 0U;
    uint32_t offset = 0U;

    /* 读取 A 区镜像大小。 */
    if (ReadAppFlagValue(App_AreaASize, &image_size) != 0U)
    {
        return 1U;
    }
    if ((0U == image_size) || (image_size > OTA_AREA_A_SIZE))
    {
        return 1U;
    }

    /* 擦除 B 区后，按子扇区复制镜像。 */
    if (extern_flash_erase(OTA_AREA_B_ADDRESS, OTA_AREA_B_SIZE) !=
        Ext_Flash_OK)
    {
        return 1U;
    }

    while (offset < image_size)
    {
        uint32_t copy_size = image_size - offset;
        if (copy_size > BOOT_READ_BUFFER_SIZE)
        {
            copy_size = BOOT_READ_BUFFER_SIZE;
        }
        if (boot_read_external(OTA_AREA_A_ADDRESS,
                               offset,
                               s_read_buffer,
                               copy_size) != 0)
        {
            return 1U;
        }
        if (extern_flash_write(OTA_AREA_B_ADDRESS + offset,
                               copy_size,
                               s_read_buffer) != Ext_Flash_OK)
        {
            return 1U;
        }
        offset += copy_size;
    }

    SetAppFlagValue(App_AreaBSize, image_size);
    return 0U;
}


/**
 * @brief 处理等待安装的新 App 镜像。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
uint8_t Boot_Manager_AppWaitUpdate(void)
{
    uint32_t image_size = 0U;

    /* 读取待安装镜像大小并检查范围。 */
    if ((ReadAppFlagValue(App_AreaASize, &image_size) != 0U) ||
        (image_size == 0U) || (image_size > MAXAPP_SIZE))
    {
        return 1U;
    }

    /* 从外部 Flash A 区解密、校验并写入 App 区。 */
    return (BootManager_BackToApp(BOOT_BLOCK_A) == 0) ? 0U : 1U;
}


/**
 * @brief 从 B 区备份镜像执行回滚。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
uint8_t Boot_Manager_Rollback_Backup_Area(void)
{
    uint32_t image_size = 0U;

    /* 读取并检查 B 区备份镜像大小。 */
    if ((ReadAppFlagValue(App_AreaBSize, &image_size) != 0U) ||
        (image_size == 0U) || (image_size > OTA_AREA_B_SIZE))
    {
        return 1U;
    }

    /* 从 B 区恢复镜像到 App 区。 */
    return (BootManager_BackToApp(BOOT_BLOCK_B) == 0) ? 0U : 1U;
}


/**
 * @brief 清除所有 App 升级标志。
 * @return void : 无返回值。
 */
static void ClearAppFlags(void)
{
    SetAppFlagValue(App_RunState, 0U);
    SetAppFlagValue(App_AreaASize, 0U);
    SetAppFlagValue(App_AreaBState, 0U);
    SetAppFlagValue(App_AreaBSize, 0U);
}


/**
 * @brief 执行 Bootloader 升级状态机。
 * @return void : 无返回值。
 */
void Boot_Manager_State(void)
{
    uint32_t state = 0U;

    /* 初始化产品相关的 AES 密钥和初始向量。 */
    if (InitAESKeyAndIV() != 0)
    {
        return;
    }

    /* 读取 App 保存的升级状态。 */
    if (ReadAppFlagValue(App_RunState, &state) != 0U)
    {
        /* 状态无效时，直接进入兜底下载流程。 */
        BootManager_BootDownload();
        return;
    }

    switch (state)
    {
        case APP_WAIT_UPDATE:
            /* A 区存在待安装镜像，先搬运到片内 App 区。 */
            if (Boot_Manager_AppWaitUpdate() == 0U)
            {
                /* 搬运成功，下一次复位后检查新 App。 */
                SetAppFlagValue(App_RunState, APP_Check_NewApp);
                /* 请求复位后优先验证并跳转到新 App。 */
                g_JumpInit = BOOT_FORCE_JUMP_MAGIC;
                boot_system_reset();
            }
            else
            {
                /* A 区安装失败，尝试从 B 区备份恢复。 */
                if (Boot_Manager_Rollback_Backup_Area() != 0U)
                {
                    /* 回滚失败，清除状态后重新启动进入兜底流程。 */
                    ClearAppFlags();
                    boot_system_reset();
                }
                /* 回滚成功时保留 Bootloader 运行状态，不立即复位。 */
            }
            break;

        case APP_Check_NewApp:
            /* 复位后检查刚写入的 App 位置大小是否正确 */
            {
                uint32_t image_size = 0U;

                /* 读取镜像长度，并检查 App 向量表栈顶地址。 */
                if ((ReadAppFlagValue(App_AreaASize,
                                              &image_size) == 0U) &&
                    (CheckAppValid((int32_t)image_size) == 0))
                {
                    /* 新 App 基础位置无误，更新运行状态并复位进入 App。 */
                    SetAppFlagValue(App_RunState, APP_Valid);
                    boot_system_reset();
                    break;
                }
            }
            if (Boot_Manager_Rollback_Backup_Area() == 0U)
            {
                /* 新 App 无效但 B 区有效，回滚后恢复运行状态。 */
                SetAppFlagValue(App_RunState, APP_Valid);
                boot_system_reset();
            }
            else
            {
                /* 新 App 和 B 区回滚都失败，清除升级状态。 */
                ClearAppFlags();
                boot_system_reset();
            }
            break;

        case APP_AreaAToAreaB:
            /* 按状态要求，把 A 区镜像复制到 B 区备份。 */
            if (Boot_Manager_AareaToBarea() == 0U)
            {
                /* 备份成功，记录 B 区有效并允许 App 运行。 */
                SetAppFlagValue(App_AreaBState, APP_AreaBState_Valid);
                SetAppFlagValue(App_RunState, APP_Valid);
                boot_system_reset();
            }
            break;

        case APP_Valid:
        default:
            {
                uint32_t image_size = 0U;
                uint32_t backup_state = 0U;
                uint8_t app_valid = 0U;

                /* 先读取当前 App 大小并检查 App 是否有效。 */
                if ((ReadAppFlagValue(App_AreaASize, &image_size) == 0U) &&
                    (CheckAppValid((int32_t)image_size) == 0))
                {
                    app_valid = 1U;
                }

                /* 状态和 App 都有效时，直接跳转运行。 */
                if ((APP_Valid == state) && (1U == app_valid))
                {
                    BootManager_JumpToApp();
                    break;
                }

                /* 当前 App 无效时，检查 B 区备份标志和镜像。 */
                if ((ReadAppFlagValue(App_AreaBState, &backup_state) == 0U) &&
                    (APP_AreaBState_Valid == backup_state) &&
                    (Boot_Manager_Rollback_Backup_Area() == 0U))
                {
                    /* 回滚成功，复位后重新进入有效 App。 */
                    SetAppFlagValue(App_RunState, APP_Valid);
                    boot_system_reset();
                    break;
                }

                /* A 区和 B 区都无效时，进入 Bootloader 兜底下载。 */
                BootManager_BootDownload();
            }
            break;
    }
}
