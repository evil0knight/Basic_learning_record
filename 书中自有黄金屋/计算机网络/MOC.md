# 计算机网络 — 知识地图

[← 学科总览](../MOC.md) | [← 主页](../../README.md)

> 状态：	OK

---

## 核心章节（按 OSI / TCP-IP 模型）

<details>
<summary><h3 style="display:inline;">🔴应用层</h3></summary>

🟢	C/S客户服务端:服务器一直开机,客户机之间不通信,一对多,好管理,安全,客户机多了会宕机

    典型:万维网网页浏览 (HTTP)、电子邮件收发 (SMTP/POP3)、文件传输 (FTP)

    P2P,一对一,不好管理,但是客户机越多,网络越快

    典型:迅雷等基于 P2P 的下载器

[DNS,域名系统](DNS.MD)

[FTP,文件传输协议](FTP,文件传输协议.md)

[电子邮件](电子邮件.MD)

[HTTP万维网](HTTP万维网.MD)

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

[组帧](组帧.MD)//字符计数,字符填充,零比特填充,

[信道划分介质访问控制]()//时分复用(TDM),频分复用(FDM),波分复用(WDM),码分复用(CDM)

[随机访问控制]()//	纯ALOHA协议,时隙ALOHA,坚持CSMA,非坚持,P非坚持,令牌

[以太网](以太网.MD)//,简单介绍局域网,4种传输介质,IEE802.3,MAC地址,MAC帧格式,CSMA/CD,IEEE802.11WIFI,CSMA/CA,以太网交换机

[广域网](广域网.MD)//PPP点对点

</details>

<details>
<summary><h3 style="display:inline">🔴物理层</h3></summary>

[性能指标](性能指标.MD)

[基础概念](基础概念.md)//码元是什么,WIFI码元8bit,奈奎斯特,香农

[编码与调制](编码与调制.md)//二进制数据,归零编码,非归零,曼彻斯特,差分曼彻斯特,幅移键控,频移键控,相移键控,正交幅度

[传输介质](传输介质.md)//双绞线,同轴电缆,光纤,无线电,红外微波激光

</details>

---
