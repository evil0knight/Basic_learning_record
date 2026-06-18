# Codex设置

[← 返回 MOC](MOC.md) | [← 主页](../../index.md) | [←AI配置与学习](AI配置与学习.md)

---

底下的都过时了

**底下的都过时了,直接装CCSWITCH:[下载链接](https://github.com/farion1231/cc-switch/releases/tag/v3.16.2)**

**有两个版本的,选安装版的,移动版的对于有些可以自动导入CCS的网站不方便**

---------2026/6/1





---



## 1. 先改 Codex 自己的配置文件

Codex 的配置文件在：

```text
C:\Users\17443\.codex\config.toml
```

我这次实际改的是下面这段：

```toml
model = "gpt-5.4"
model_provider = "spatialai"

[model_providers.spatialai]
name = "SpatialAI"
base_url = "https://spatialai.vip/"
env_key = "OPENAI_API_KEY"
wire_api = "responses"
```

这段配置的意思是：

- `model = "gpt-5.4"`：默认模型用 `gpt-5.4`
- `model_provider = "spatialai"`：让 Codex 走自定义 provider
- `base_url`：指向中转站地址
- `env_key = "OPENAI_API_KEY"`：API Key 不写死在文件里，而是从环境变量里读取
- `wire_api = "responses"`：按兼容的 Responses 接口方式通信

---

## 2. API Key 不建议明文写进配置文件

推荐把 Key 放到系统用户环境变量里，不要直接写进 `config.toml` 或项目仓库。

PowerShell 永久设置：

```powershell
[Environment]::SetEnvironmentVariable("OPENAI_API_KEY","你的API_KEY","User")
```

设置后要：重启VSCODE

---

## 3. 如何测试是否配置成功

我这次实际测试用的是：

```bash
codex exec "Reply with exactly: ok"
```

如果返回：

```text
ok
```

说明 Codex 已经能正常连到你配置的接口。
