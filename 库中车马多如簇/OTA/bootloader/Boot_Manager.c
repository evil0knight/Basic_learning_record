#include "Boot_Manager.h"
#include <string.h>
#include "ota_adapter.h"
#include "ota_config.h"
#include "ota_flash_handler.h"
#include "stm32f411_flash_handler.h"
#include "AES.h"
#include "md5.h"
#include "ymodem.h"

/*
 * BootLoader 升级状态机
 *
 * 依赖映射：
 *   - 内部 Flash 擦写                                 -> stm32f411_flash_handler_erase/write/read
 *   - 外部 Flash 读写                                 -> OTA 块管理（W25Q64_ReadData/WriteData）
 *   - AES 密钥/IV                                     -> ota_config.h 的 OTA_AES_KEY_BYTES/IV_BYTES
 *   - AES 解密                                        -> AES 库
 *   - MD5 校验                                        -> md5 库
 *   - 软复位                                          -> ota_adapter_system_reset
 *   - 跳转（VTOR/MSP/复位向量）                        -> ota_adapter_jump_to_app
 */

static uint8_t s_read_buffer[BLOCK_SIZE];

/* 上电跳转标志，放在 SRAM 末尾，软复位不会清除（0x55AA55AA = 升级成功后强制跳转）。
 * Keil ARMCC 用 __attribute__((at(addr))) 固定地址；换 GCC 需改用 section + 链接脚本。 */
uint32_t g_JumpInit __attribute__((at(0x2001FFF0)));

/* 校验 APP 区向量表（栈指针在 RAM 范围 + 复位向量为 Thumb） */
static int8_t CheckAppValid(uint32_t image_size)
{
    uint8_t vector[8];
    uint32_t stack_top, reset;

    if ((image_size < 8U) || (image_size > OTA_APP_MAX_SIZE))
        return -1;

    if (stm32f411_flash_handler_read(OTA_APP_ADDRESS, vector, sizeof(vector)) != STM32F411_FLASH_OK)
        return -1;

    stack_top = (uint32_t)vector[0] | ((uint32_t)vector[1] << 8U) |
                ((uint32_t)vector[2] << 16U) | ((uint32_t)vector[3] << 24U);
    reset = (uint32_t)vector[4] | ((uint32_t)vector[5] << 8U) |
            ((uint32_t)vector[6] << 16U) | ((uint32_t)vector[7] << 24U);

    if ((stack_top & 0x2FFE0000U) != 0x20000000U || ((reset & 1U) == 0U))
        return -1;

    return 0;
}

/*
 * 从外部 Flash 的 block_index（BLOCKA/BLOCKB）按扇区读密文，
 * AES-256 CBC 解密后写入内部 Flash APP 区，并校验尾部 16 字节 MD5。
 * 调用前需先 SetBlockParmeter(block_index, size) 设置该块有效长度。
 * 返回 0 成功。
 */
static int8_t BootManager_BackToApp(uint8_t block_index)
{
    uint8_t temp_block[16];
    /* AES 密钥/IV 直接取 config 静态配置（TODO[换产品]: 安全芯片方案在此改为从 KEROS 读） */
    static const uint8_t key[OTA_AES_KEY_SIZE] = OTA_AES_KEY_BYTES;
    static const uint8_t iv[OTA_AES_BLOCK_SIZE] = OTA_AES_IV_BYTES;
    uint8_t expected_md5[16], computed_md5[16];
    MD5_CTX md5_ctx;
    uint32_t app_size, processed = 0U, md5_collected = 0U;
    uint16_t read_memory_size = 0U, read_memory_index = 0U;
    uint16_t read_time = 0U, read_data_count = 0U;

    /* 先读一帧，解析密文头得到 app_size（含尾部 16 字节 MD5） */
    if (W25Q64_ReadData(block_index, s_read_buffer, &read_memory_size) != 0U)
        return 1;

    if (read_memory_size < 16U)
        return 1;

    memcpy(temp_block, s_read_buffer, 16U);
    Aes_IV_key256bit_Decode(iv, temp_block, key);
    app_size = ((uint32_t)temp_block[15] << 24U) | ((uint32_t)temp_block[14] << 16U) |
               ((uint32_t)temp_block[13] << 8U)  |  (uint32_t)temp_block[12];

    if ((app_size < 16U) || (app_size > (OTA_APP_MAX_SIZE + 16U)))
        return 1;

    /* 擦 APP 区（只擦应用数据部分 app_size - 16） */
    if (stm32f411_flash_handler_erase(OTA_APP_ADDRESS, app_size - 16U) != STM32F411_FLASH_OK)
        return 1;

    MD5Init(&md5_ctx);
    read_data_count = app_size / 16U + ((app_size % 16U) != 0U ? 1U : 0U);
    read_memory_index = 16U; /* 跳过已消费的密文头 */

    for (read_time = 0U; read_time < read_data_count; read_time++)
    {
        uint32_t app_data_size, remain_app, feed_len;

        /* 当前扇区读完则读下一扇区 */
        if (read_memory_index == read_memory_size)
        {
            if (W25Q64_ReadData(block_index, s_read_buffer, &read_memory_size) != 0U)
                return 1;
            read_memory_index = 0U;
        }

        memcpy(temp_block, s_read_buffer + read_memory_index, 16U);
        read_memory_index += 16U;

        Aes_IV_key256bit_Decode(iv, temp_block, key);

        /* 计算当前块中属于应用数据的字节数（总大小含 MD5，应用数据为 app_size-16） */
        app_data_size = app_size - 16U;
        remain_app = (app_data_size > processed) ? (app_data_size - processed) : 0U;
        feed_len = (remain_app >= 16U) ? 16U : remain_app;

        /* 应用数据：喂 MD5 + 写内部 Flash APP 区 */
        if (feed_len > 0U)
        {
            MD5Update(&md5_ctx, temp_block, feed_len);
            if (stm32f411_flash_handler_write(OTA_APP_ADDRESS + processed,
                                              temp_block, feed_len) != STM32F411_FLASH_OK)
                return 1;
            processed += feed_len;
        }

        /* 应用数据不足一整块时，块内剩余字节属于 MD5 起始段 */
        if (remain_app < 16U && md5_collected < 16U)
        {
            uint32_t md5_take = 16U - feed_len;
            memcpy(&expected_md5[md5_collected], temp_block + feed_len, md5_take);
            md5_collected += md5_take;
        }
    }

    MD5Final(&md5_ctx, computed_md5);
    if (md5_collected != 16U)
        return 1;

    return (memcmp(computed_md5, expected_md5, 16U) == 0) ? 0 : 1;
}

/* A 区 -> B 区备份（外部 Flash 之间复制） */
static uint8_t Boot_Manager_AareaToBarea(void)
{
    uint32_t a_size = 0U;
    uint16_t read_len = 0U;

    if (read_app_flag_value(App_AreaASize, &a_size) != 0U)
        return 1;
    if (a_size > OTA_AREA_A_SIZE)
        return 1;

    SetBlockParmeter(BLOCKA, a_size);

    for (;;)
    {
        uint8_t ret = W25Q64_ReadData(BLOCKA, s_read_buffer, &read_len);
        if (ret == 0U)
        {
            if (W25Q64_WriteData(BLOCKB, s_read_buffer, read_len) != 0U)
                return 1;
        }
        else if (ret == 1U)
        {
            break; /* 读完 */
        }
        else
        {
            return 1;
        }
    }

    (void)W25Q64_WriteData_End(BLOCKB);
    set_app_flag_value(App_AreaBSize, a_size);
    return 0;
}

/* 从 B 区解密回 APP 区（回退） */
static uint8_t Boot_Manager_Rollback_Backup_Area(void)
{
    uint32_t b_size = 0U;

    if (read_app_flag_value(App_AreaBSize, &b_size) != 0U)
        return 1;
    if (b_size > OTA_AREA_B_SIZE)
        return 1;

    SetBlockParmeter(BLOCKB, b_size);
    return (BootManager_BackToApp(BLOCKB) == 0) ? 0 : 1;
}

/* APP_WAIT_UPDATE：解密搬运并校验，成功返回 0 */
static uint8_t Boot_Manager_AppWaitUpdate(void)
{
    uint32_t a_size = 0U;

    if (read_app_flag_value(App_AreaASize, &a_size) != 0U)
        return 1;
    if (a_size > OTA_APP_MAX_SIZE)
        return 1;

    SetBlockParmeter(BLOCKA, a_size);
    if (BootManager_BackToApp(BLOCKA) != 0)
        return 1;

    return (CheckAppValid(a_size) == 0) ? 0 : 1;
}

/* 清除所有标志 */
static void clear_all_flags(void)
{
    set_app_flag_value(App_RunState, 0U);
    set_app_flag_value(App_AreaBSize, 0U);
    set_app_flag_value(App_AreaBState, 0U);
    set_app_flag_value(App_AreaASize, 0U);
}

/* 跳转到 APP */
void BootManager_JumpToApp(void)
{
    uint32_t run_state = 0U;

    /* 升级成功后软复位：强制跳转（此时 APP 已写好，无需再看标志） */
    if (g_JumpInit == 0x55AA55AA)
    {
        g_JumpInit = 0xFFFFFFFF;
        ota_adapter_watchdog_reinit();
        ota_adapter_jump_to_app(OTA_APP_ADDRESS);
    }
    else if (read_app_flag_value(App_RunState, &run_state) == 0U &&
             run_state == APP_Valid)
    {
        ota_adapter_jump_to_app(OTA_APP_ADDRESS);
    }
}

/* BootLoader 升级状态机（上电执行，成功跳转 APP） */
void Boot_Manager_State(void)
{
    uint32_t run_state = 0U;

    W25Q64_Init();
    (void)read_app_flag_value(App_RunState, &run_state);

    switch (run_state)
    {
        case APP_WAIT_UPDATE:
            if (Boot_Manager_AppWaitUpdate() == 0U)
            {
                set_app_flag_value(App_RunState, APP_Check_NewApp);
                ota_adapter_system_reset();
            }
            else
            {
                /* 升级失败，回退 B 区 */
                if (Boot_Manager_Rollback_Backup_Area() != 0U)
                {
                    clear_all_flags();
                    ota_adapter_system_reset();
                }
            }
            break;

        case APP_Check_NewApp:
            /* 一般不会进入，进入说明上次升级失败，需回退 */
            if (Boot_Manager_Rollback_Backup_Area() == 0U)
            {
                set_app_flag_value(App_RunState, APP_Valid);
                ota_adapter_system_reset();
            }
            else
            {
                clear_all_flags();
                ota_adapter_system_reset();
            }
            break;

        case APP_AreaAToAreaB:
            /* 保留当前 A 区数据备份到 B 区 */
            if (Boot_Manager_AareaToBarea() == 0U)
            {
                set_app_flag_value(App_AreaBState, APP_AreaBState_Valid);
                set_app_flag_value(App_RunState, APP_Valid);
                ota_adapter_system_reset();
            }
            else
            {
                set_app_flag_value(App_AreaBState, 0U);
            }
            break;

        case APP_Valid:
        default:
            BootManager_JumpToApp();
            break;
    }
}

/* BootLoader 内直接下载：Ymodem 数据写外部 Flash A 区 */
static int32_t boot_download_sink(const uint8_t *data, uint32_t size,
                                  uint32_t address, void *context)
{
    (void)address;
    (void)context;
    return (W25Q64_WriteData(BLOCKA, (uint8_t *)data, size) == 0U) ? 0 : -1;
}

/*
 * BootLoader 内直接 Ymodem 接收下载 -> 解密搬运 -> 校验 -> 跳转。
 * 无有效 App 时由 main 循环里触发。
 */
void BootManager_BootDownload(void)
{
    int32_t fil_size;

    W25Q64_Init();

    /* Ymodem 接收，sink 直接写外部 Flash A 区 */
    fil_size = Ymodem_ReceiveWithSink(s_read_buffer, boot_download_sink,
                                      NULL, OTA_AREA_A_ADDRESS);
    if (fil_size <= 0)
        return;

    /* 冲刷最后不足一个扇区的数据 */
    if (W25Q64_WriteData_End(BLOCKA) != 0U)
        return;

    /* 从 A 区解密搬运到内部 Flash APP 区（write_index 已由下载累积，无需 SetBlockParmeter） */
    if (BootManager_BackToApp(BLOCKA) != 0)
        return;

    if (CheckAppValid((uint32_t)fil_size) != 0)
        return;

    set_app_flag_value(App_AreaASize, (uint32_t)fil_size);
    set_app_flag_value(App_AreaBSize, (uint32_t)fil_size);
    set_app_flag_value(App_AreaBState, APP_AreaBState_Valid);
    set_app_flag_value(App_RunState, APP_Check_NewApp);

    /* 设置上电跳转标志，软复位后 BootManager_JumpToApp 强制跳转 */
    g_JumpInit = 0x55AA55AA;
    ota_adapter_system_reset();
}
