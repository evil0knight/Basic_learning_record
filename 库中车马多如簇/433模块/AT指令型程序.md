# 433 UART AT 指令型程序

[← 返回 MOC](MOC.md) | [← 主页](../../index.md)

---

## 上电初始化与 AT 参数配置（适合模块首次烧录/固定参数场景）

这套写法来自 `CAT2PRO` 工程的 `Modules/radio.c`、`Modules/radio.h`、`App/main.c`。
MCU 先初始化 UART，再通过串口向 433 模块发送 AT 指令，配置发射功率、串口波特率、频点、扩频因子、带宽、前导码和周期休眠参数。
配置完成后发送 `ATW` 保存参数，最后拉低休眠控制脚让模块进入持续工作状态。

```c
#include "SYSCFG.h"

// ---------- UART 基础发送 ----------
void UART_SendByte(unsigned char data)
{
    TXEF    = 0;
    URDATAL = data;
    while(!TCF) {}
}

void UART_SendATCmd(const char *cmd)
{
    unsigned char j = 0;
    while(cmd[j] != '\0') {
        UART_SendByte(cmd[j]);
        j++;
    }
    UART_SendByte(0x0D);
    UART_SendByte(0x0A);
}

// ---------- 433 模块控制引脚 ----------
#define CONFIG_RADIO_ADDR_VIA_AT
#define SELF_ADR        0xFFFF

#define RADIO_SLEEP()   do { PC1 = 1; } while(0)
#define RADIO_WAKE()    do { PC1 = 0; } while(0)

void Radio_Wake(void)
{
    RADIO_WAKE();
}

void Radio_Sleep(void)
{
    RADIO_SLEEP();
}

// ---------- UART 初始化：9600 baud, PA6 TX / PA7 RX ----------
void UART_Init(void)
{
    PCKEN |= 0B00100000;
    URIER  = 0B00000001;
    URLCR  = 0B00000001;
    URMCR  = 0B00011000;
    URDLL  = 104;
    URDLH  = 0;
    TCF    = 1;
    AFP1   = 0;
    ODCON0 = 0B00000000;
}

// ---------- 433 AT 参数初始化 ----------
void Radio_Init(void)
{
#ifdef CONFIG_RADIO_ADDR_VIA_AT
    UART_SendATCmd("AT+PWR=20");
    DelayMs(50);

    UART_SendATCmd("AT+UART=3,0");
    DelayMs(50);

    UART_SendATCmd("AT+FEQ=434000000");
    DelayMs(50);

    UART_SendATCmd("AT+SF=10");
    DelayMs(50);

    UART_SendATCmd("AT+BW=9");
    DelayMs(50);

    UART_SendATCmd("AT+PB=2000");
    DelayMs(50);

    UART_SendATCmd("AT+MODE=1");
    DelayMs(50);

    UART_SendATCmd("AT+WT=2");
    DelayMs(50);

    // UART_SendATCmd_ADR(SELF_ADR);
    DelayMs(50);

    UART_SendATCmd("ATW");
    DelayMs(50);
#endif

    Radio_Wake();
}

// ---------- 上电初始化 ----------
void POWER_INITIAL(void)
{
    OSCCON = 0B01110001;
    INTCON = 0B00000000;

    PORTA = 0B00000000;
    TRISA = 0B10010000;
    PORTB = 0B00000000;
    TRISB = 0B10000101;
    PORTC = 0B00000000;
    TRISC = 0B00000000;

    WPUA = 0B00010000;
    WPUB = 0B10000101;
    WPUC = 0B00000000;

    WPDA = 0B00000000;
    WPDB = 0B00000000;
    WPDC = 0B00000000;

    PSRC0 = 0B11111111;
    PSRC1 = 0B11111111;
    PSRC2 = 0B11111111;

    PSINK0 = 0B11111111;
    PSINK1 = 0B11111111;
    PSINK2 = 0B11111111;

    ANSELA = 0B00000000;
}

void main(void)
{
    POWER_INITIAL();
    UART_Init();
    Radio_Init();
    INTCON = 0B11000000;

    while(1) {
    }
}
```

---

## 串口协议收发与对码处理（适合主控与执行器之间做帧通信）

这套写法使用 UART 中断解析自定义协议帧：`[0xFF][src][func][data...][0xFE]`。
支持阀门开关命令、对码帧解析，以及对码成功后把地址写入 433 模块 AT 参数。
执行器动作完成后，还会主动回传连接状态或阀门状态。

```c
#include "SYSCFG.h"

#define ADDR_MAIN     0xAAu
#define ADDR_SUB      0xABu
#define ADDR_SELF     0xACu

#define FUNC_PAIR     0x01u
#define FUNC_TIME     0x02u
#define FUNC_VALVE    0x03u
#define FUNC_CONNECT  0x04u
#define FUNC_BATTERY  0x05u

typedef enum {
    WAIT_HDR,
    RX_SRC,
    RX_FUNC,
    RX_DATA,
    RX_TAIL
} ParserState;

typedef enum {
    EV_NONE = 0,
    EV_CMD_OPEN,
    EV_CMD_CLOSE,
    EV_PAIR_SUCCESS
} SystemEvent;

volatile SystemEvent g_system_evt = EV_NONE;

void FSM_SendEvent(SystemEvent evt)
{
    g_system_evt = evt;
}

void UART_SendByte(unsigned char data)
{
    TXEF    = 0;
    URDATAL = data;
    while(!TCF) {}
}

void UART_SendATCmd(const char *cmd)
{
    unsigned char j = 0;
    while(cmd[j] != '\0') {
        UART_SendByte(cmd[j]);
        j++;
    }
    UART_SendByte(0x0D);
    UART_SendByte(0x0A);
}

static unsigned char uint_to_str(unsigned int val, char *buf)
{
    char tmp[6] = {0};
    unsigned char i = 0, j = 0;
    if(val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    while(val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    while(i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
    return j;
}

void UART_SendATCmd_ADR(unsigned int addr)
{
    char buf[20] = "AT+ADR=";
    uint_to_str(addr, buf + 7);
    UART_SendATCmd(buf);
}

static unsigned char func_data_len(unsigned char func)
{
    switch(func) {
        case FUNC_PAIR:    return 5;
        case FUNC_VALVE:   return 2;
        case FUNC_CONNECT: return 2;
        default:           return 0xFF;
    }
}

void UART_ISR(void)
{
    static ParserState   ps       = WAIT_HDR;
    static unsigned char rx_buf[8];
    static unsigned char data_idx = 0;
    static unsigned char data_len = 0;

    if(URRXNE && RXNEF) {
        unsigned char b = URDATAL;

        switch(ps) {
            case WAIT_HDR:
                if(b == 0xFF) {
                    ps = RX_SRC;
                }
                break;

            case RX_SRC:
                if(b == ADDR_SUB) {
                    ps = WAIT_HDR;
                    break;
                }
                rx_buf[0] = b;
                ps = RX_FUNC;
                break;

            case RX_FUNC:
                data_len = func_data_len(b);
                if(data_len == 0xFF) {
                    ps = WAIT_HDR;
                    break;
                }
                rx_buf[1] = b;
                data_idx  = 0;
                ps = (data_len > 0) ? RX_DATA : RX_TAIL;
                break;

            case RX_DATA:
                rx_buf[2 + data_idx] = b;
                if(++data_idx >= data_len) {
                    ps = RX_TAIL;
                }
                break;

            case RX_TAIL:
                if(b == 0xFE) {
                    if(rx_buf[1] == FUNC_VALVE) {
                        if(rx_buf[2] == 0x00 && rx_buf[3] == 0x01) {
                            FSM_SendEvent(EV_CMD_OPEN);
                        } else if(rx_buf[2] == 0x00 && rx_buf[3] == 0x00) {
                            FSM_SendEvent(EV_CMD_CLOSE);
                        }
                    } else if(rx_buf[1] == FUNC_PAIR) {
                        unsigned int addr = 0;
                        unsigned char i;
                        for(i = 0; i < 5; i++) {
                            addr = addr * 10 + rx_buf[2 + i];
                        }
                        UART_SendATCmd_ADR(addr);
                        FSM_SendEvent(EV_PAIR_SUCCESS);
                    }
                }
                ps = WAIT_HDR;
                break;

            default:
                ps = WAIT_HDR;
                break;
        }
    }
}

static void UART_SendFrame(unsigned char func, const unsigned char *data, unsigned char len)
{
    unsigned char i;
    UART_SendByte(0xFF);
    UART_SendByte(ADDR_SELF);
    UART_SendByte(func);
    for(i = 0; i < len; i++) {
        UART_SendByte(data[i]);
    }
    UART_SendByte(0xFE);
}

void UART_SendConnect(void)
{
    unsigned char data[2] = {0x00, 0x00};
    UART_SendFrame(FUNC_CONNECT, data, 2);
}

void UART_SendValveStatus(unsigned char is_open)
{
    unsigned char data[2];
    data[0] = 0x00;
    data[1] = is_open ? 0x01 : 0x00;
    UART_SendFrame(FUNC_VALVE, data, 2);
}

void interrupt ISR(void)
{
    if(URRXNE && RXNEF) {
        UART_ISR();
        NOP();
    }
}
```

---
