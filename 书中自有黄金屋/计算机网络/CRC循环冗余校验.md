# CRC 循环冗余校验

[← 返回 MOC](MOC.md)|[← 返回 Modbus](../../术中自有万钟粟/Modbus.md)

---

## 原理

发送方在数据后附加 **FCS（帧校验序列/冗余码）**，接收方验证能否被生成多项式整除。

核心运算是**模 2 除法**：加减法均不进位不借位，即用 **XOR** 代替加减。详见 [→ 模2除法](模2除法.md)

---

## 计算步骤

设数据为 M，生成多项式 G（最高次为 r）：

1. 在 M 后补 **r 个 0**（相当于 M × 2^r）
2. 用补零后的数据对 G 做**模 2 除法**，得余数 R（共 r 位，不足补前导 0）
3. 将 R 替换末尾的 r 个 0，得到发送帧 T = M·2^r XOR R
4. 接收方用 T 对 G 做模 2 除法，**余数为 0 → 无误**，否则出错

---

CRC16半查表法,[Modbus_RTU](../../术中自有万钟粟/Modbus.md)中使用

```c
const unsigned short wCRCTalbeAbs[] =
{
        0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400,
};

unsigned short mt_api_crc16(unsigned char *ptr, unsigned int len) 
{
        unsigned short wCRC = 0xFFFF;
        unsigned short i;
        unsigned char chChar;
        unsigned char temp[2];
        for (i = 0; i < len; i++)
        {
                chChar = *ptr++;
                wCRC = wCRCTalbeAbs[(chChar ^ wCRC) & 15] ^ (wCRC >> 4);
                wCRC = wCRCTalbeAbs[((chChar >> 4) ^ wCRC) & 15] ^ (wCRC >> 4);
        }
        temp[0] = wCRC&0xFF; 
        temp[1] = (wCRC>>8)&0xFF;
        wCRC = (temp[0]<<8)|temp[1];
        return wCRC;
}
```

## 要点

- **只能检错，不能纠错**（知道出错但不知道哪里错）
- **生成多项式越长，检错能力越强**
- 常用标准：CRC-8（G = x⁸+x²+x+1）、CRC-16、**CRC-32**（以太网帧用）
- CRC 能检出所有奇数个错、所有长度 ≤ r 的突发错误

---

## 本章小结
