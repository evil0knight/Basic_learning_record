# DWT 区间测量

[← MOC](../MOC.md) | [← 主页](../../index.md)
[← DWT 移植](../../术中自有万钟粟/debug方法/DWT.md)

---

## 文件

| 文件                            | 内容                                             |
| ------------------------------- | ------------------------------------------------ |
| [dwt_measure.h](./dwt_measure.h) | 接口、状态枚举、分频与基线宏                     |
| [dwt_measure.c](./dwt_measure.c) | 解锁并自检计数器、起点锁存、终点减基线、分频打印 |

## 用法

```c
#include "dwt_measure.h"

dwt_measure_enter();
/* 被测代码，可跨函数或文件 */
dwt_measure_exit();
```

- 首次 `enter()` 自动使能 `DEMCR.TRCENA` 与 `DWT.CYCCNTENA`，无需手动初始化、无需定义变量。
- `exit()` 锁存终点后扣除固定基线并直接打印，打印耗时不计入本次结果。
- 单通道：不可嵌套、不可并发，重复 `enter()` 会覆盖旧起点。

## 输出

```text
DWT_MEASURE raw=292 overhead=30 cycles=262 us=1.541 max=310 max_us=1.823
```

## 移植

1. 复制 `dwt_measure.h`、`dwt_measure.c` 到工程的中间件目录并加入编译，`.c` 里不写死器件头。
2. 模板不包含任何工程配置头。分频优先取 `CFG_FEATURE_MEASURE_DIV`，没有则按 1；工程要统一配置就在包含本头文件前自行定义。
3. 默认打印走 `printf`。要换成 RTT 日志等通道，在包含头文件前定义 `DWT_MEASURE_PRINTF(fmt, ...)`。
4. 可按需覆盖 `DWT_MEASURE_SAMPLE_DIV`、`DWT_MEASURE_OVERHEAD_CYCLES`（换芯片换工程换编译器等就重新测）
   把被测代码删掉，DIV=1，连着跑

   ```
   for (;;) { dwt_measure_enter(); dwt_measure_exit(); }
   ```

   看打印的 raw，取稳定值

## 支持的内核

只依赖内核自带的 `DWT->CYCCNT`，不含任何厂商外设。

| 内核                                  | 情况                                                                   |
| ------------------------------------- | ---------------------------------------------------------------------- |
| M3 / M4 / M7 / M33 / M35P / M55 / M85 | 支持，刻度就是 CPU 主频                                                |
| M7 / M33 及以上                       | 寄存器默认锁定，首次 `enter()` 自动写 `DWT->LAR = 0xC5ACCE55` 解锁 |
| M0 / M0+ / M23                        | 没有 CYCCNT，编译期直接 `#error`，只能用 SysTick 或外设定时器        |
| 任意内核                              | `SystemCoreClock` 必须是真实主频，运行时改频要同步更新它             |

器件头两个来源，二选一：先包含工程的 CMSIS 器件头，或编译选项定义
`-DDWT_MEASURE_DEVICE_HEADER='"stm32h7xx.h"'`。
首次 `enter()` 会做一次计数自检（连读两次 `CYCCNT` 必须变化），
自检不过返回 `DWT_MEASURE_NO_COUNTER`，不会静默输出 0。

## 测量点由工程侧定义

测哪个函数、用哪个开关宏，属于板级/工程级配置，不进本库。做法是用工程自己的宏把 `enter()`/`exit()` 包起来：

```c
#ifdef MEASURE_ADC_TIME
    dwt_measure_enter();
#endif
    adc_read();
#ifdef MEASURE_ADC_TIME
    dwt_measure_exit();
#endif
```

- 宏名和组合由工程自定，同一时间只允许开一个（互斥用 `#error` 卡住）。
- 分频值 `DWT_MEASURE_SAMPLE_DIV` 可跟工程配置走，配置头里改一次就同时管到测量开关和打印频率。

## 相关笔记

- [DWT 原理](../../术中自有万钟粟/Cortex-M4内核原理/DWT.md)：CYCCNT 与其他 DWT 功能
- [DWT 移植](../../术中自有万钟粟/debug方法/DWT.md)：最小可用移植步骤
