# OTA

[← 返回库的目录](../MOC.md) | [← 主页](../../index.md)

OTA 升级中间件：APP 端后台下载，BootLoader 端解密安装与回退。

## 文件结构

| 文件                                                                                              | 作用                              | 移植时            |
| ------------------------------------------------------------------------------------------------- | --------------------------------- | ----------------- |
| [ota_config.h](ota_config.h)                                                                         | 地址 / 密钥 / 尺寸                | 换产品改这里      |
| [ota_adapter.h](ota_adapter.h) / [ota_adapter.c](ota_adapter.c)                                         | 复位 / 跳转 / UI / 命令接收       | 换芯片/产品改这里 |
| [service_ota_manager.h](app/service_ota_manager.h) / [service_ota_manager.c](app/service_ota_manager.c) | APP 端状态机 + 标志区读写 + 下载  | 不改              |
| [ota_flash_handler.h](app/ota_flash_handler/ota_flash_handler.h) / [ota_flash_handler.c](app/ota_flash_handler/ota_flash_handler.c) | 外部 Flash 块管理（A/B 双块读写） | 不改              |
| [Boot_Manager.h](bootloader/Boot_Manager.h) / [Boot_Manager.c](bootloader/Boot_Manager.c)               | BootLoader 升级状态机             | 不改              |

## 如何使用

### 1. 申请资源（目标工程）

OTA 需要的任务 / 队列 / 互斥量在目标工程的 [user_task_reso_config](../代码架构/firmware/01_app/app_init/MOC.md) 里申请，填写步骤见该笔记的「user_task_reso_config配置」一节。

**① rtos_config.h 加资源参数：**

```c
#define CFG_TASK_OTA_STACK_SIZE          512U
#define CFG_TASK_OTA_PRIORITY            48U
#define CFG_TASK_OTA_FLASH_STACK_SIZE    512U
#define CFG_TASK_OTA_FLASH_PRIORITY      49U
#define CFG_QUEUE_OTA_APP_DATA_DEPTH     2U
```

**② user_task_reso_config.h 加索引枚举（`IDX_MAX` 放最后）：**

```c
typedef enum
{
    /* 已有索引... */
    USER_TASK_IDX_OTA,
    USER_TASK_IDX_OTA_FLASH,
    USER_TASK_IDX_MAX
} user_task_idx_t;

typedef enum
{
    /* 已有索引... */
    USER_QUEUE_IDX_OTA_APP_DATA,
    USER_QUEUE_IDX_MAX
} user_queue_idx_t;

typedef enum
{
    /* 已有索引... */
    USER_MUTEX_IDX_OTA_EXT_FLASH,
    USER_MUTEX_IDX_MAX
} user_mutex_idx_t;
```

**③ user_task_reso_config.c 填配置表：**

```c
user_task_config_t g_user_tasks[USER_TASK_IDX_MAX] =
{
    /* 启用, 名称, 入口, 栈大小, 优先级, 参数, 句柄 */
    {true, "OTA_task",       ota_task_runnable,                CFG_TASK_OTA_STACK_SIZE,       CFG_TASK_OTA_PRIORITY,       NULL, NULL},
    {true, "DwAppData_task", download_app_data_task_runnable,  CFG_TASK_OTA_FLASH_STACK_SIZE, CFG_TASK_OTA_FLASH_PRIORITY, NULL, NULL}
};

user_queue_config_t g_user_queues[USER_QUEUE_IDX_MAX] =
{
    /* 启用, 深度, 单条消息大小, 句柄 */
    {true, CFG_QUEUE_OTA_APP_DATA_DEPTH, sizeof(uint8_t *), NULL}
};

user_mutex_config_t g_user_mutexes[USER_MUTEX_IDX_MAX] =
{
    /* 启用, 句柄 */
    {true, NULL}
};
```

### 2. APP 端流程（从发命令到复位）

APP 端有两个任务：

- **OTA_task**（入口 `ota_task_runnable`）：跑状态机。
- **DwAppData_task**（入口 `download_app_data_task_runnable`）：下载消费者，把数据写外部 Flash。

状态机 4 个状态：`WaitReqDownload` → `OtaDownload` → `WaitReqUpdate` → `OtaEnd`。

#### 第 1 步：等下载命令（状态 WaitReqDownload，初始态）

- `ota_task_runnable` 循环在 `WaitReqDownload`。
- 调 `ota_adapter_ui_state_write(0)`：UI 显示「空闲」。
- 调 `ota_adapter_wait_download_request()`：**阻塞等 `0x11 0x22 0x33` 命令**。
  - 这是弱函数，产品层要覆盖实现（例如 UART 空闲中断收 4 字节，匹配 `0x11 0x22 0x33` 返回 1）。
  - 收到命令（返回非 0）→ 进入第 2 步；没收到就一直等。

#### 第 2 步：等用户确认下载（仍在 WaitReqDownload 状态内）

- 调 `ota_adapter_ui_state_write(1)`：UI 显示「是否下载？」。
- 调 `wait_confirmation()`：轮询 `ota_adapter_ui_confirm_read()`，等用户点确认。
  - `ota_adapter_ui_confirm_read()` 返回 `0`=等待中、`1`=确认、`2`=取消。
  - **用户确认（返回 1）** → 状态切到 `OtaDownload`，调 `ui_state_write(0)`。
  - **取消 / 超时（100 秒）** → 回到 `WaitReqDownload` 继续等命令。

#### 第 3 步：下载（状态 OtaDownload）

- 调 `ota_adapter_ui_progress_write(0)`：进度 0。
- 调 `W25Q64_Init()`：初始化 A/B 块管理上下文。
- 调 `Ymodem_ReceiveWithSink(...)`：Ymodem 收数据，**每个数据块走 `ymodem_queue_sink` 回调**：
  1. sink 把数据复制到双缓冲槽 → 发槽指针到队列 `USER_QUEUE_IDX_OTA_APP_DATA`；
  2. sink 阻塞握手（拿 `USER_MUTEX_IDX_OTA_EXT_FLASH` 互斥量再释放），等消费者写完；
  3. 消费者 `DwAppData_task`（`download_app_data_task_runnable`）从队列拿指针 → 持互斥量 → `W25Q64_WriteData(BLOCKA, 指针, g_u32_datalength)` **写外部 Flash A 区** → 释放互斥量。
- 调 `ota_adapter_ui_progress_write(100)`：进度 100。
- 成功（`appDataLength > 0` 且无写错误）：
  - `W25Q64_WriteData_End(BLOCKA)`：冲刷最后不足 4KB 的数据。
  - `set_app_flag_value(App_AreaASize, appDataLength)`：写 A 区大小标志。
  - `set_app_flag_value(App_RunState, APP_WAIT_UPDATE)`：**写运行状态 = 等待升级**。
  - 状态切到 `WaitReqUpdate`。
- 失败：回到 `WaitReqDownload`。

#### 第 4 步：等用户确认安装（状态 WaitReqUpdate）

- 调 `ota_adapter_ui_state_write(2)`：UI 显示「下载完成，是否安装？」。
- 调 `ota_adapter_ui_confirm_write(0)`：清确认标志。
- 调 `wait_confirmation()`：等用户确认。
  - **确认（1）** → 状态切到 `OtaEnd`。
  - **取消 / 超时** → 回到 `WaitReqDownload`（标志还是 `APP_WAIT_UPDATE`，BootLoader 上电会继续处理）。

#### 第 5 步：复位（状态 OtaEnd）

- 调 `osal_task_delay(1000ms)`：延时 1 秒。
- 调 `SoftReset()` → `ota_adapter_system_reset()`：**NVIC 软复位，进入 BootLoader**。

### 3. BootLoader 端（软复位后）

#### 上电第一件事：`main()` 调 `BootManager_JumpToApp()`

- `g_JumpInit == 0x55AA55AA`？→ 升级成功后软复位，**强制跳转 APP**（不看标志）。
- 读 `App_RunState == APP_Valid`？→ **正常跳转 APP**。
- 都不是 → 继续 bootloader 初始化，进 `Boot_Manager_State()`。

#### `Boot_Manager_State()`：读标志续跑状态机

- 调 `W25Q64_Init()`：初始化块管理。
- 调 `read_app_flag_value(App_RunState, &run_state)`：读运行状态标志。
- 按 `run_state` 走：

| run_state 值                      | 走哪个函数                              | 做什么                                                                                                                        | 成功后                           | 失败后                                              |
| --------------------------------- | --------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------- | -------------------------------- | --------------------------------------------------- |
| `APP_WAIT_UPDATE` (0x11111111)  | `Boot_Manager_AppWaitUpdate()`        | 读 A 区密文 →`BootManager_BackToApp(BLOCKA)` AES 解密 → 写内部 Flash APP 区 → MD5 校验 → `CheckAppValid()` 校验向量表 | 设 `APP_Check_NewApp` + 软复位 | `Boot_Manager_Rollback_Backup_Area()` 从 B 区回退 |
| `APP_Check_NewApp` (0x22222222) | `Boot_Manager_Rollback_Backup_Area()` | 从 B 区解密回退到 APP 区                                                                                                      | 设 `APP_Valid` + 软复位        | 清所有标志 + 软复位                                 |
| `APP_AreaAToAreaB` (0x33333333) | `Boot_Manager_AareaToBarea()`         | A 区复制到 B 区（备份）                                                                                                       | 设 `APP_Valid` + 软复位        | 设 B 区状态 0                                       |
| `APP_Valid` (0x44444444) / 其他 | `BootManager_JumpToApp()`             | 跳转 APP                                                                                                                      | -                                | -                                                   |

## 如何移植

换产品 / 换芯片时：地址 / 密钥在 [ota_config.h](ota_config.h) 里配，复位 / 跳转 / UI / 命令接收在 [ota_adapter.c](ota_adapter.c) 里实现。

依赖：[Ymodem](../Ymodem/MOC.md)、[Flash](../Flash/MOC.md)、[AES](../AES/MOC.md)、[MD5](../MD5/MOC.md)、[看门狗](../看门狗/MOC.md)。
