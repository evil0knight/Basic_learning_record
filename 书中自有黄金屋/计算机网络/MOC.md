# 计算机网络 — 知识地图

[← 学科总览](../MOC.md) | [← 主页](../../README.md)

> 状态：🔄 进行中（约 2/3）

---

## 核心章节（按 OSI / TCP-IP 模型）

<details>
<summary><h3 style="display:inline;">🔴应用层</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴传输层</h3></summary>

[传输层的服务	](传输层服务.MD)总览传输层基本框架

[UDP协议](UDP协议.md)

[TCP协议](TCP协议.md)

</details>

<details>
<summary><h3 style="display:inline">🔴网络层</h3></summary>

#### [IP多播](IP多播.md)

<details>
<summary><h4 style="display:inline">移动IP</h4></summary>

    IP数据先到永久地址，后转发到漫游位置，用IP套娃发送数据报，移动站发不需要转发

    一个外地地址会有多个移动站使用，外地的信号站会直接用MAC转发

</details>

<details>
<summary><h4 style="display:inline">网络层设备,路由器构造</h4></summary>

    路由器的构造：路由选择（路由表（网络前缀，下一跳，出接口，其他），RIP,OSPF,BGP,）

    分组转发（转发表（网络前缀，下一跳，出接口），MAC,出接口等物理层，数据链路层）

</details>

</details>

<details>
<summary><h3 style="display:inline">🔴数据链路层</h3></summary>

[奇偶校验码](奇偶校验码.md)：1位校验位，只能检测奇数个错误，不能纠错

[CRC循环冗余校验](CRC循环冗余校验.md)：生成多项式 + [模2除法](模2除法.md)，检错能力强，以太网常用CRC-32

[海明码](海明码.md)：插入多个校验位，能定位并纠正1位错误

[滑动窗口](滑动窗口.md):可靠传输机制

</details>

<details>
<summary><h3 style="display:inline">🔴物理层</h3></summary>

</details>

---

## 高频考点

- TCP 三次握手 / 四次挥手
- HTTP vs HTTPS
- 拥塞控制

## 错题 / 难点
