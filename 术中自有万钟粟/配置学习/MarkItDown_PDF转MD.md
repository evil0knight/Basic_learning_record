# MarkItDown PDF转MD

[← 返回 MOC](MOC.md) | [← 主页](../../index.md)

---

## 1. 最常用法

我平时只需要把 PDF 转成 Markdown，直接记下面两步就够了。

先在 PowerShell 里补一下当前会话的环境变量：

```powershell
$env:PYTHONPATH='E:\PythonTools\markitdown'
```

然后执行转换：

```powershell
markitdown ".\xxx.pdf" -o ".\xxx.md"
```

例如：

```powershell
markitdown ".\书中自有黄金屋\提问的智慧.pdf" -o ".\书中自有黄金屋\提问的智慧.md"
```

如果想先直接看输出，不写文件：

```powershell
markitdown ".\xxx.pdf"
```

---

## 2. 检查是否可用

先执行：

```powershell
$env:PYTHONPATH='E:\PythonTools\markitdown'
```

再检查版本：

```powershell
markitdown --version
```

我这台机器当前实测结果是：

```text
markitdown 0.1.5
```

---

## 3. 下载与安装

官方最简单的安装命令就是一条：

```powershell
pip install markitdown[all]
```

这个命令和你当前在 `C:` 还是 `D:` 没关系，终端开在哪个盘都能装，关键是这台机器的 `pip` 要能用。

按我现在电脑里的实际安装结果看，当时更像是装到了单独目录里，位置是：

```text
E:\PythonTools\markitdown
```

如果以后想按现在这台机器的方式重新装，大概率可以直接用：

```powershell
pip install markitdown[all] --target E:\PythonTools\markitdown
```

如果 `markitdown` 命令还没进 PATH，可以临时补一下：

```powershell
$env:Path += ';E:\PythonTools\markitdown\bin'
```

---

## 4. 当前机器的实际情况

- `markitdown.exe` 在 `E:\PythonTools\markitdown\bin\markitdown.exe`
- 包主体在 `E:\PythonTools\markitdown`
- 当前直接运行 `markitdown` 会因为少了 `PYTHONPATH` 报错
- 先执行 `$env:PYTHONPATH='E:\PythonTools\markitdown'` 后即可正常使用

---

## 5. 常见情况

如果报这个错：

```text
ModuleNotFoundError: No module named 'markitdown'
```

一般不是没装，而是当前终端没有补：

```powershell
$env:PYTHONPATH='E:\PythonTools\markitdown'
```

如果转换 PDF 时看到 `ffmpeg` 或 `FontBBox` 之类警告，可以先不管。我这次实测时有这些提示，但 PDF 仍然能正常转出 Markdown 文件。
