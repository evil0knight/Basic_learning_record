# WSL快速上手

[← 返回 Linux系统](./MOC.md) | [← 返回技术栈](../MOC.md)

---

## 1. 先怎么进入

Windows 终端或 PowerShell 里直接输入:

```powershell
wsl
```

如果你装了多个发行版, 先看列表:

```powershell
wsl -l -v
```

再进入指定系统:

```powershell
wsl -d Ubuntu
```

---

## 2. 进去后常用什么

```bash
pwd
ls
cd ~
mkdir -p ~/code
sudo apt update
sudo apt upgrade -y
```

- `cd ~`: 回到 Linux 主目录
- `~/code`: 建议你放项目的位置

---

## 3. 文件放哪里

如果你主要在 WSL 里开发, 项目尽量放在 Linux 文件系统里:

```bash
cd ~/code
```

不建议长期把项目放在 `/mnt/c` 或 `/mnt/d` 里跑开发环境。

---

## 4. 配合 VS Code

先装 `Remote - WSL` 扩展, 然后在项目目录里执行:

```bash
code .
```

这样会直接用 WSL 环境打开当前项目。

如果想在 Windows 资源管理器里打开当前目录:

```bash
explorer.exe .
```

---

## 5. 退出和关闭

```bash
exit
```

```powershell
wsl --shutdown
```

- `exit`: 退出当前 Linux
- `wsl --shutdown`: 关闭全部 WSL

---

安装、迁移这些内容见: [WSL与Linux环境](../配置学习/WSL与Linux环境.md)
