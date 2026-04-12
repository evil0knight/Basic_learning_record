# Codex设置

[← 返回 MOC](MOC.md) | [← 主页](../../index.md) | [←AI配置与学习](AI配置与学习.md)

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

设置后要：

1. 关闭当前终端
2. 重新打开终端
3. 再运行 `codex`

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

---

## 4. 如果接口报错怎么办

有些 OpenAI 兼容站点要求地址必须带 `/v1`，如果当前地址不通，可以把：

```toml
base_url = "https://spatialai.vip/"
```

改成：

```toml
base_url = "https://spatialai.vip/v1"
```

如果你使用的是内置 OpenAI provider，也常见写法是：

```toml
openai_base_url = "https://xxx.xxx/v1"
```

但我这次采用的是**自定义 provider**方案，兼容性更直接一点。

---

## 5. VS Code 里的 settings.json 要不要配

可以保留：

```json
"OPENAI_BASE_URL": "https://spatialai.vip/"
```

但更关键的是：

- `config.toml` 里 provider 要配对
- `OPENAI_API_KEY` 最好走系统环境变量

如果把 Key 明文写进：

```text
C:\Users\17443\AppData\Roaming\Code\User\settings.json
```

虽然能用，但不够安全，容易出现在：

- 聊天记录
- 编辑器历史
- 本地日志
- 配置同步

---

## 6. 这次实际完成了什么

### 已完成

- 已修改 `C:\Users\17443\.codex\config.toml`
- 已把 Codex provider 指到 `https://spatialai.vip/`
- 已通过命令测试连通性

### 测试命令

```bash
codex exec "Reply with exactly: ok"
```

### 测试结果

```text
ok
```

说明当前配置已经可用。
