# OTA

[← 返回模块总览](../MOC.md) | [← 返回主页](../../index.md)
[→ 术中自有万钟粟 OTA](../../术中自有万钟粟/OTA/MOC.md)

---

OTA 由 App 服务和 Bootloader 两部分组成：

App 负责接收并保存升级镜像

Bootloader 在复位后检查升级标志，将镜像搬运到 App 区并完成跳转

## 子模块

- [OTA App 服务](./app/MOC.md)：运行期间的下载状态机、升级标志和外部 Flash 缓存。
- [OTA Bootloader](./bootloader/MOC.md)：上电升级状态判断、镜像安装、有效性检查和 App 跳转。
