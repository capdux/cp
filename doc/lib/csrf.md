# `cap:csrf` — CSRF 防护标准库

## 1. 概述

`lib/csrf/`（导入协议 `cap:csrf` 或动态挂载 `cap.csrf`）提供纯 CP 实现的工业级跨站请求伪造（CSRF）防护能力：
- **CSPRNG + HMAC-SHA256 签名令牌**：基于 `std.crypto.random` 生成 16 字节随机 nonce 与 32 字节密钥，绑定时间戳与长度前缀编码的 `session_id`，使用 `std.crypto.timing_safe.eql` 执行常量时间验签，支持可选 TTL 过期控制。
- **抗 BREACH 随机掩码令牌（Masked Token）**：每次输出随机 32 字节 pad 与 `(pad ^ raw_token)` 的 Base64URL 无填充编码，彻底防范 HTTP 压缩侧信道泄露。
- **Origin / Referer / Sec-Fetch-Site 防御策略**：支持精确 `scheme:host:port` 解析与比对、IPv6 主机支持、`null` Origin 与 userinfo 欺骗拦截、`Sec-Fetch-Site: cross-site` 拦截及安全 HTTP 方法判定。

## 2. 核心类型与常量

- `CsrfError`: 错误集合，包含 `InvalidToken`、`ExpiredToken`、`InvalidSignature`、`DecodingFailed`、`InvalidOrigin`、`CrossSiteRequest`、`MissingOrigin`、`InvalidSecret`、`OutOfMemory`。
- `CsrfConfig`: 默认配置结构体（`max_age_ms = 86_400_000`、`require_origin_on_unsafe = true`、`allow_referer_fallback = true`）。
- `CsrfProtector`: 封装密钥与 `max_age_ms` 的保护器结构体，提供 `.init`、`.initWithMaxAge`、`.sign`、`.verify`、`.mask`、`.unmaskAndVerify`。
- `ParsedOrigin`: `{ scheme: []const u8, host: []const u8, port: u16 }`。
- 常量：`DEFAULT_EXPIRATION_MS` (24h)、`SECRET_LEN` (32)、`NONCE_LEN` (16)、`TIMESTAMP_LEN` (8)、`MAC_LEN` (32)、`RAW_TOKEN_LEN` (56)、`MASK_PAD_LEN` (32)。

## 3. API 速查

| 函数 / 方法 | 签名摘要 | 说明 |
|---|---|---|
| `generateSecret` | `(out_buf: *[32]u8): []u8` | 生成 32B CSPRNG 密钥 |
| `generateSecretAlloc` | `(alloc): ![]u8` | 堆分配生成 32B CSPRNG 密钥 |
| `signToken` / `signTokenAt` | `(alloc, secret, session_id[, ts_ms]): ![]u8` | 生成 Base64URL HMAC-SHA256 签名令牌 |
| `verifySignedToken` | `(secret, session_id, token): !void` | 常量时间验签并按默认 24h TTL 校验 |
| `verifySignedTokenWithTtl` | `(secret, session_id, token, max_age_ms): !void` | 自定义 TTL 验签（`0` 表示跳过过期检查） |
| `maskToken` | `(alloc, raw_token): ![]u8` | 生成 32B 随机 pad + XOR 掩码的 Base64URL 令牌 |
| `unmaskAndVerify` / `...WithTtl` | `(alloc, secret, session_id, masked[, ttl]): !void` | 解码去掩码并执行常量时间验签 |
| `isSafeMethod` | `(method: []const u8): bool` | 判断是否为 `GET`/`HEAD`/`OPTIONS`/`TRACE` |
| `parseOrigin` / `originsMatch` | `(url): ParsedOrigin?` / `(a, b): bool` | 严格解析并比对 `scheme:host:port` |
| `verifyOrigin` | `(origin, referer, expected, sec_fetch_site): !void` | 综合校验 `Sec-Fetch-Site`、`Origin` 与 `Referer` |

## 4. 示例

```cp
import cap from "cap";
import { generateSecret, signToken, maskToken, unmaskAndVerify, verifyOrigin } from "cap:csrf";

pub fn handlePost(alloc: std.mem.Allocator, session_id: []const u8): !void {
    let secret_buf: [32]u8 = [0; 32];
    const secret = generateSecret(&secret_buf);

    const raw = try signToken(alloc, secret, session_id);
    defer alloc.free(raw);

    const masked = try maskToken(alloc, raw);
    defer alloc.free(masked);

    try unmaskAndVerify(alloc, secret, session_id, masked);
    try verifyOrigin("https://app.example.com", null, "https://app.example.com", "same-origin");
    const _ = cap.csrf.isSafeMethod("GET");
}
```
