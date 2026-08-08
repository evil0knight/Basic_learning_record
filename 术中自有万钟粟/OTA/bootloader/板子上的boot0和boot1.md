
# 芯片烧录模式与 BOOT 引脚（ICP / ISP / IAP）

[← bootloader](./MOC.md) | [← 主页](../../../index.md)

---

## 一、 ICP（In-Circuit Programming - 在电路编程）

- **BOOT 引脚配置**：`BOOT0 = 0`，`BOOT1 = 0`（或任意）
- **启动源**：主 Flash（Main Flash，起始地址 `0x0800 0000`）
- **核心机制**：
  - 芯片正常运行主 Flash 中的程序。
  - 依靠外部硬件调试器（如 ST-Link、J-Link、DAP-Link）通过物理调试接口（SWD / JTAG）直接接管 CPU 内核，对 Flash 进行读写与在线调试。
- **特点与场景**：
  - 权限最高，即使 Flash 中无程序也可擦写。
  - 适用于日常开发编译下载、断点调试及工厂批量贴片烧录。

---

## 二、 ISP（In-System Programming - 在系统编程）

- **BOOT 引脚配置**：`BOOT0 = 1`
- **核心机制**：芯片硬件强制绕过主 Flash 的用户代码，根据 `BOOT1` 电平的不同，进入以下两种特定模式：

### 情况 1：BOOT1 = 0（系统存储器模式 / 串口救砖）

- **启动源**：系统存储器（System Memory / ROM，起始地址 `0x1FFF F000`）
- **工作原理**：
  - CPU 启动并运行芯片出厂时**物理固化在 ROM 中的官方 Bootloader**（只读，无法被破坏或擦除）。
  - 官方 Bootloader 自动监听串口（USART1）、USB 等接口，配合上位机软件（如 FlyMcu、STM32CubeProgrammer）完成固件接收与 Flash 重写。
- **适用场景**：
  - 用户 APP 代码崩溃、死锁或误禁用 SWD/JTAG 引脚导致“变砖”时的**串口救援**。
  - 无仿真器/调试器情况下的串口下载。

### 情况 2：BOOT1 = 1（内置 SRAM 模式 / 内存运行）

- **启动源**：内置 SRAM（内存，起始地址 `0x2000 0000`）
- **工作原理**：
  - 仿真器将编译好的代码直接灌入 RAM，CPU 直接在 RAM 中读取并执行指令。
  - 完全不经过 Flash 擦写，不消耗 Flash 寿命，**掉电后代码彻底丢失**。
- **适用场景**：
  - 频繁修改算法的极速免擦写调试。
  - Flash 擦写算法自身的验证。

---

## 三、 IAP（In-Application Programming - 在应用编程）

- **BOOT 引脚配置**：`BOOT0 = 0`，`BOOT1 = 0`（硬件物理引脚保持主 Flash 启动）
- 原理与实操都在[IAP实操+原理.md](IAP实操+原理.md)
