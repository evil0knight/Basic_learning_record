# FreeRTOS 编程风格

[←主页](../../index.md)|[← FreeRTOS内核实现与应用开发实战指南知识地图](./MOC.md)

---

## 数据类型

对C的标准数据类型进行了重定义:
`char-->portCHAR,`

`long-->portBASE_TYPE`(根据处理器架构来决定多少位)/`portLONG`,

port是接口的意思,这些在 `portmacro.h`里设置

## 变量名

`char`类型的变量前缀是c,`short`类型的变量前缀是s,`long`类型的变量前缀是l

`portBASE_TYPE`类型的变量前缀是x,包括数据结构,任务句柄,队列句柄等前缀都是x

## 函数名

如果是私有函数,会加 `prv`(private)前缀

函数名由:   返回值  +  所在文件  +  函数名称  组成

1)`vTaskPrioritySet()`,函数返回值为 `void`,在 `task.c`中定义

2)`xQueueReceive()`,函数返回值为 `portBASE_TYPE`,在 `queue.c`中定义

3)`vSemaphoreCreateBinary()`,函数返回值为 `void`,在 `semaphr.h`中定义

## 宏

宏由大写的字母表示,并配有**小写字母的前缀**,前缀用于表示**该宏在那个头文件定义**

1)`portMAX_DELAY`,宏定义在 `portable.h`

2)`taskENTER_CRITICAL()`,宏定义在 `task.h`

3)`pdTRUE`,宏定义在 `projdefs.h`

## 格式

1Tab==4空格
