# Windows目录联接迁移缓存

[← 返回 配置学习](./MOC.md) | [← 主页](../../index.md)

---

## 作用

Windows 目录联接（Directory Junction）适合把硬编码在 C 盘的缓存、索引、聊天记录迁移到 D/E 盘。

| 特性 | 说明 |
| ---- | ---- |
| 透明重定向 | 软件继续读写原 C 盘路径，系统自动把 I/O 导向目标盘 |
| C 盘零体积 | C 盘只保留几十字节的入口，真实数据落在 D/E 盘 |
| 历史数据不断档 | 先移动旧数据，再建立联接，软件体验不变 |

---

## 标准三步法

### 1. 彻底关闭目标软件

- 退出托盘程序
- 任务管理器结束残留进程
- 防止缓存文件被占用，导致移动或删除失败

### 2. 迁移数据并清空 C 盘实体目录

- 将 `C:\原缓存路径` 内全部内容移动到 `D:\AppCache\软件名称`
- 确认目标盘数据完整
- 删除 C 盘同名实体文件夹

关键点：联接路径不能已经是普通文件夹，必须先删除原 C 盘实体目录。

### 3. 建立 Junction

PowerShell 推荐写法：

```powershell
# 确保目标目录存在
New-Item -ItemType Directory -Path "D:\AppCache\软件名称" -Force | Out-Null

# 建立目录联接
New-Item -ItemType Junction -Path "C:\原缓存路径" -Target "D:\AppCache\软件名称"
```

CMD 写法：

```dos
mklink /J "C:\原缓存路径" "D:\AppCache\软件名称"
```

---

## 高频缓存路径

| 软件类别 | C 盘原路径 | 建议目标路径 |
| ---- | ---- | ---- |
| VS Code C/C++ 索引 | `C:\Users\用户名\AppData\Local\Microsoft\vscode-cpptools` | `D:\AppCache\vscode-cpptools` |
| 飞书客户端 | `C:\Users\用户名\AppData\Roaming\LarkShell` | `D:\AppCache\Feishu` |
| 网易云音乐 | `C:\Users\用户名\AppData\Local\NetEase` | `D:\AppCache\NetEase` |
| Chrome 浏览器缓存 | `C:\Users\用户名\AppData\Local\Google\Chrome\User Data\Default\Cache` | `D:\AppCache\Chrome` |
| AI Agent | `C:\Users\用户名\.hermes` / `C:\Users\用户名\.codex` | `E:\Toolchains\hermes-home` / `E:\Toolchains\codex-home` |

---

## 验证

查看目录是否带 `<JUNCTION>` 标识：

```powershell
cmd /c dir /a "C:\Users\用户名\AppData\Local\Microsoft" | findstr /i "vscode-cpptools"
```

| 输出状态 | 说明 |
| ---- | ---- |
| `<JUNCTION>  目录名 [目标盘路径]` | 生效 |
| `<DIR>  目录名` | 未生效，仍是普通实体文件夹 |

---

## 维护避坑

| 场景 | 做法 |
| ---- | ---- |
| 删除软链接 | 删除 C 盘联接入口即可，只会移除链接关系 |
| 清理缓存内容 | 进入目标盘目录删除内部文件 |
| 路径带空格 | 命令两端必须加双引号，例如 `"User Data"` |
| 重新迁移 | 先删旧联接入口，再按三步法重建 |
| 不确定是不是链接 | 先用 `dir /a` 检查，别直接删目标盘目录 |
