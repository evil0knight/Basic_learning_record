# 计算机网络 — 知识地图

[← 学科总览](../MOC.md) | [← 主页](../../README.md)

> 状态：🔄 进行中（约 2/3）

---

## 核心章节（按 OSI / TCP-IP 模型）

<details>
<summary><h3 style="display:inline">应用层</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">传输层</h3></summary>

#### [传输层的服务	](传输层服务.MD)总览传输层基本框架

#### [UDP协议](UDP协议.md)


</details>

<details>
<summary><h3 style="display:inline">网络层</h3></summary>

#### [IP多播](IP多播.md)

    发送一份，遇节点被复制转发，仅限目的地址

#### 移动IP

    IP数据先到永久地址，后转发到漫游位置，用IP套娃发送数据报，移动站发不需要转发

    一个外地地址会有多个移动站使用，外地的信号站会直接用MAC转发

#### 网络层设备

    路由器的构造：路由选择（路由表（网络前缀，下一跳，出接口，其他），RIP,OSPF,BGP,）

    分组转发（转发表（网络前缀，下一跳，出接口），MAC,出接口等物理层，数据链路层）

</details>

<details>
<summary><h3 style="display:inline">数据链路层</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">物理层</h3></summary>

</details>

---

## 高频考点

- TCP 三次握手 / 四次挥手
- HTTP vs HTTPS
- 拥塞控制

## 错题 / 难点
