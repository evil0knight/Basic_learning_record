# HTTP 报文首部

[← 返回 MOC](MOC.md) | [← 主页](../../index.md) | [← 返回HTTP报文](HTTP报文.md)

---

HTTP 首部字段分为四类：**通用首部**（请求和响应都能用）、**请求首部**（只在请求中出现）、**响应首部**（只在响应中出现）、**实体首部**（描述报文主体）。

## 通用首部字段

| 首部字段名 | 说明 |
|---|---|
| `Cache-Control` | 控制缓存行为（如 `no-cache`、`max-age=秒数`） |
| `Connection` | 管理连接（`keep-alive` / `close`），也用于控制哪些首部不转发给代理 |
| `Date` | 报文创建的日期时间 |
| `Pragma` | HTTP/1.0 遗留的缓存控制（`no-cache`），向后兼容用 |
| `Trailer` | 说明报文末尾有哪些首部字段（分块传输时用） |
| `Transfer-Encoding` | 传输报文主体时的编码方式（如 `chunked`） |
| `Upgrade` | 请求升级到其他协议（如 WebSocket） |
| `Via` | 追踪请求/响应经过的代理节点 |
| `Warning` | 缓存相关的警告信息 |

## 请求首部字段

| 首部字段名 | 说明 |
|---|---|
| `Accept` | 客户端能处理的媒体类型及优先级（如 `text/html,application/json`） |
| `Accept-Charset` | 客户端支持的字符集及优先级 |
| `Accept-Encoding` | 客户端支持的内容编码（如 `gzip`、`deflate`） |
| `Accept-Language` | 客户端偏好的自然语言（如 `zh-CN,en`） |
| `Authorization` | 客户端的认证凭据（如 Bearer Token） |
| `Expect` | 期望服务器的特定行为（如 `100-continue`） |
| `From` | 发出请求的用户的邮件地址 |
| `Host` | 请求的目标主机名和端口（HTTP/1.1 必须携带） |
| `If-Match` | 条件请求：ETag 匹配时才执行 |
| `If-Modified-Since` | 条件请求：资源在指定时间后有修改才返回 |
| `If-None-Match` | 条件请求：ETag 不匹配时才执行（常用于缓存验证） |
| `If-Range` | 条件请求：ETag 或日期匹配时才做范围请求 |
| `If-Unmodified-Since` | 条件请求：资源在指定时间后未修改才执行 |
| `Max-Forwards` | 限制请求经过的最大代理/网关跳数（TRACE/OPTIONS 用） |
| `Proxy-Authorization` | 代理服务器要求的认证凭据 |
| `Range` | 请求实体的字节范围（断点续传） |
| `Referer` | 请求来源页面的 URI |
| `TE` | 客户端支持的传输编码及优先级 |
| `User-Agent` | 发起请求的客户端程序信息 |

## 响应首部字段

| 首部字段名 | 说明 |
|---|---|
| `Accept-Ranges` | 服务器是否支持范围请求（`bytes` / `none`） |
| `Age` | 响应在代理缓存中存放的时间（秒） |
| `ETag` | 资源的唯一标识符，用于缓存验证 |
| `Location` | 重定向的目标 URI |
| `Proxy-Authenticate` | 代理服务器要求客户端认证的方式 |
| `Retry-After` | 告知客户端多久后可以再次发请求 |
| `Server` | 服务器软件信息 |
| `Vary` | 代理缓存时，哪些请求首部影响缓存版本 |
| `WWW-Authenticate` | 服务器要求客户端认证的方式（401 响应时携带） |

## 实体首部字段

| 首部字段名 | 说明 |
|---|---|
| `Allow` | 资源支持的 HTTP 方法列表（405 响应时携带） |
| `Content-Encoding` | 实体主体的内容编码方式（如 `gzip`） |
| `Content-Language` | 实体主体的自然语言 |
| `Content-Length` | 实体主体的字节长度 |
| `Content-Location` | 实体主体对应的 URI |
| `Content-MD5` | 实体主体的 MD5 摘要，用于完整性校验 |
| `Content-Range` | 实体主体的字节范围（配合范围请求使用） |
| `Content-Type` | 实体主体的媒体类型（如 `text/html; charset=UTF-8`） |
| `Expires` | 实体主体的过期日期时间 |
| `Last-Modified` | 资源最后修改的日期时间 |

## Cookie 相关首部

Cookie 不属于 HTTP/1.1 规范，但使用极为普遍，单独列出。

| 首部字段名 | 出现位置 | 说明 |
|---|---|---|
| `Set-Cookie` | 响应 | 服务器向客户端写入 Cookie |
| `Cookie` | 请求 | 客户端将保存的 Cookie 发回服务器 |

`Set-Cookie` 常见属性：

| 属性 | 说明 |
|---|---|
| `expires` | Cookie 的有效期；省略则为会话 Cookie（关闭浏览器即失效） |
| `path` | 限制 Cookie 发送范围的文件目录 |
| `domain` | Cookie 适用的域名（可跨子域） |
| `Secure` | 仅在 HTTPS 连接时发送 |
| `HttpOnly` | 禁止 JavaScript 读取，防止 XSS 窃取 Cookie |
