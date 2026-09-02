# 外设库模板

[← 模块总览](../MOC.md)|[←主页](../../index.md)

[xxx知识](./知识/MOC.md)

> 本模板由 Flash 库抽象而来。以后在库里新增一个外设（IIC 器件、SPI 器件、EEPROM、传感器、显示屏等），**复制本目录，把 `xxx` 改成外设名**，再按各层注释填空实现。
>
> 注意：Flash 是「最完整形态」，不是每个外设都要五层。先判断你的外设属于哪一类（见下），再决定保留哪几层。

<!-- 复制后删除：模板提醒，一次性判断用 -->

## 先判断：你的外设属于哪一类

| 类               | 分层                             | 什么时候用                           | 典型外设                             |
| ---------------- | -------------------------------- | ------------------------------------ | ------------------------------------ |
| A（同步）        | Wrapper + Adapter + Driver + Reg | 单次操作原子、快速、不需要后台线程   | 气压、心率、hp4570、屏幕、磁力、触摸 |
| B（异步）        | A 再加 Handler                   | 需要事件队列 / 独立线程串行处理      | 温湿度、运动传感器 mpu6050、喇叭     |
| C（异步 + 服务） | B 再加 Service                   | 需要分区 / 异步读写服务 / 事件组同步 | Flash（storage）                     |

**必有层（A 类起跳）**：Wrapper、Adapter、Driver、Reg。
**按需层**：Handler（要异步 / 队列 / 线程才加）、Service（要分区 / 异步服务才加）。

> 裸机（无 RTOS）：不用 Wrapper / Handler / Service，只用 Driver，在 `bsp_adapter_port_xxx` 里定义 `XXX_BARE_METAL` 宏切换成同步调用（`xxx_bare_init` / `xxx_bare_read` / `xxx_bare_write`）。参考 Flash 的 [裸机使用 Driver](../Flash/裸机使用Driver.md)。

<!-- 复制后删除：模板目录说明，新外设的目录结构见「文件结构」表 -->

## 目录结构

```
外设库/
├── MOC.md                       # 模块总览 + 接入教程（照 Flash/MOC.md 写）
├── 知识/                        # 【必建】协议/时序/读写笔记（见 知识/MOC.md）
├── bsp_wrapper_xxx/             # 【必有】Wrapper：函数表注册，纯转发，无 OS/器件依赖
├── bsp_adapter_port_xxx/        # 【必有】Adapter：接口绑定 + 发事件 / 同步调（XXX_BARE_METAL 切换）
├── xxx_device/                  # 【必有】器件层：Driver（寄存器）+ Reg（命令码）
│   ├── bsp_xxx_driver.h/.c      #    Driver
│   ├── bsp_xxx_reg.h            #    Reg
│   └── bsp_xxx_handler.h/.c     # 【按需 B/C】Handler：事件驱动 + 单任务
└── service_xxx_manage/          # 【按需 C】Service：异步读写服务 + 事件组 + 单任务
    ├── service_xxx_manage.h/.c
    └── xxx_manage_port.h/.c
```

<!-- 复制后删除：思路提醒，写代码时参考即可 -->

## 通用模式（照抄，不要改思路）

**必有（A 类起跳）：**

1. **函数表注册**：Wrapper 只存 `xxx_drv_t` 函数表，通过 `bsp_wrapper_xxx_reg()` 注册(**自己那个 `xxx/bsp_wrapper_xxx/bsp_wrapper_xxx.c`文件里的静态数组 `_xxx_drv_dev[]`**)，上层永远不直接碰 Driver。
2. **接口注入解耦**：Driver 的「总线接口」是函数指针结构体，由 Adapter 注入，器件层不 include 任何 OS / 总线头文件。

**按需（B/C 类才用）：**

3. **事件驱动**：Adapter 把上层请求打包成事件，塞进 Handler 的事件队列，由 Handler 单任务取出处理。
4. **单任务串行 = 天然互斥**：所有硬件访问都在一个任务里串行执行，不需要 mutex。
5. **事件组同步**：Service 层用事件组「请求位 + 完成位」在单任务里串行处理，对外接口阻塞等待完成。

<!-- 复制后保留：移植清单，改代码时反复查 -->

### argument 传入（B/C 类要改的文件）

1. 改 [bsp_adapter_port_xxx.c](./bsp_adapter_port_xxx/bsp_adapter_port_xxx.c)：定义并初始化全局变量 `xxx_handler_all_input_arg`，把四个接口指针填进去（总线 / 时基 / OS 队列 / 延时）。
2. 改 [user_task_reso_config.h](../代码架构/firmware/01_app/app_init/user_task_config/user_task_reso_config.h)：在 `user_task_idx_t` 枚举加 `USER_TASK_IDX_XXX_DRV`。
3. 改 [user_task_reso_config.c](../代码架构/firmware/01_app/app_init/user_task_config/user_task_reso_config.c)：在 `g_user_tasks[]` 里给 Handler 任务那行的 argument 字段填 `&xxx_handler_all_input_arg`。
4. 完成信号量：改 [user_task_reso_config.h](../代码架构/firmware/01_app/app_init/user_task_config/user_task_reso_config.h) 加 `USER_SEMA_IDX_XXX`，改 [user_task_reso_config.c](../代码架构/firmware/01_app/app_init/user_task_config/user_task_reso_config.c) 的 `g_user_semaphores[]` 加一行，代码里 `user_sema_get(USER_SEMA_IDX_XXX)` 拿句柄。

<!-- 复制后删除：复制动作说明，做完即删 -->

## 复制改名步骤

1. 判断外设类别（A / B / C），决定保留哪几层；
2. 复制本目录，改名 `新外设/`，删掉不需要的层目录；
3. 全局把 `xxx` 替换成外设名（如 `eeprom`、`sensor`）；
4. 按顺序填空：
   - `xxx_device/bsp_xxx_reg.h`：填命令码 / 寄存器定义；
   - `xxx_device/bsp_xxx_driver.c`：填寄存器读写实现；
   - `xxx_device/bsp_xxx_handler.c`（B/C）：按外设补事件类型和分发；
   - `bsp_adapter_port_xxx.c`：绑定总线 Port 和 OSAL 接口；
   - `service_xxx_manage.c`（C）：按外设补读写/擦除等对外接口；
5. 写 `MOC.md`：照 [Flash/MOC.md](../Flash/MOC.md) 的结构（文件结构+位置、申请资源、配置宏、注册流程、可复用解耦、依赖）。

<!-- 复制后保留：移植清单，逐文件怎么处理 -->

## 哪些是骨架照抄、哪些要改

| 文件                        | 必需性   | 处理方式                                                            |
| --------------------------- | -------- | ------------------------------------------------------------------- |
| `bsp_wrapper_xxx.h/.c`    | 必有     | 骨架照抄，函数表成员按外设增删                                      |
| `bsp_adapter_port_xxx.h`  | 必有     | 骨架照抄，配置宏按外设改                                            |
| `bsp_adapter_port_xxx.c`  | 必有     | 总线绑定按外设改；OS 发事件 / 裸机同步调（`XXX_BARE_METAL` 切换） |
| `bsp_xxx_driver.h/.c`     | 必有     | 接口注入照抄，寄存器实现按外设写                                    |
| `bsp_xxx_reg.h`           | 必有     | 全部按外设写                                                        |
| `bsp_xxx_handler.h/.c`    | 按需 B/C | 事件队列 + 单任务照抄，事件类型 / 分发按外设改                      |
| `service_xxx_manage.h/.c` | 按需 C   | 事件组 + 单任务照抄，对外接口按外设改                               |
| `xxx_manage_port.h/.c`    | 按需 C   | 骨架照抄，转发目标按外设改                                          |

> A 类（同步）外设删掉 Handler 和 Service 两个目录，只用 Wrapper → Adapter → Driver 的同步调用链；Driver 的 `pf_read/write` 直接同步返回结果，不需要事件和信号量。
