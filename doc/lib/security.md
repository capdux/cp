# CP 标准库：应用安全与鉴权引擎手册 (`cap:security`)

> 参考来源：`/data/dev/bun/src/csrf/csrf.cp`, `/data/dev/bun/docs/runtime/utils.mdx` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:security` 是 CP 语言官方内置的应用安全与鉴权标准库，提供纯 CP 实现的 **CSRF 无状态令牌**、**RFC 4226/6238 HOTP/TOTP 双因素认证与 RFC 4648 Base32**、**Fetch CORS 跨域策略引擎**、以及 **CSP 策略构造器与 OWASP 安全响应头预设**。全部模块 0 `std.posix`、0 `@import("bun")`。

### 1. 模块文件拓扑

| 文件路径 | 溯源注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/security/csrf.cp` | `// Source: /data/dev/bun/src/csrf/csrf.cp` | `CSRF` 令牌签发与验证、`TokenFormat`、`DEFAULT_EXPIRATION_MS`、`constantTimeEql` |
| `lib/security/totp.cp` | `// Source: cp/lib/security/totp.cp (RFC 4226 HOTP, RFC 6238 TOTP & RFC 4648 Base32)` | `Base32`、`OtpAlgorithm`、`Hotp`、`TotpConfig`、`Totp` |
| `lib/security/cors.cp` | `// Source: cp/lib/security/cors.cp (Fetch CORS Policy Engine)` | `CorsConfig`、`CorsDecision`、`CorsPolicy`、`CorsHeader`、`CorsDecisionKind`、`CorsRejectReason` |
| `lib/security/csp.cp` | `// Source: cp/lib/security/csp.cp (Content-Security-Policy & Security Headers)` | `CspBuilder`、`SecurityHeadersPreset`、`SecurityHeaderSet`、`SecurityHeader` |
| `lib/security/index.cp` | `// Source: /data/dev/bun/src/csrf/csrf.cp` | `cap:security` 对外统一导出入口 |

---

## 二、 CSRF 令牌与恒定时间比对 (`csrf.cp`)

每个由 `cap:security` 签发的 CSRF Token 在编码前均由固定 **64 字节（512 bits）** 二进制结构组成：

```text
 0                   8                        24                  32                                  64
 ┌───────────────────┬────────────────────────┬───────────────────┬───────────────────────────────────┐
 │   Timestamp (ms)  │     CSPRNG Nonce       │  ExpiresIn (ms)   │       HMAC-SHA256 Signature       │
 │  u64 Big-Endian   │   16 Bytes Random      │  u64 Big-Endian   │     32 Bytes over Bytes [0..32]   │
 └───────────────────┴────────────────────────┴───────────────────┴───────────────────────────────────┘
```

- `constantTimeEql(a, b)`: 恒定时间按位异或比对，防止时序侧信道。
- `CSRF.generate` / `CSRF.verify`: 支持 `TokenFormat.Base64Url`、`Base64`、`Hex`。
- `CSRF.generateRaw` / `generateRawAt` / `verifyRaw` / `verifyRawAt`: 零堆分配 64 字节二进制生成与带时钟偏移/溢出防回绕保护的确定性校验。

---

## 三、 RFC 4648 Base32 与 RFC 4226/6238 HOTP / TOTP (`totp.cp`)

### 1. `Base32`
- `Base32.encode(alloc, data)` / `Base32.encodeNoPad(alloc, data)`: 标准 RFC 4648 大写字母表编码（可选 `=` 填充）。
- `Base32.decode(alloc, encoded)`: 容错解码，自动忽略 ASCII 空白（空格、`\t`、`\r`、`\n`）、大小写不敏感、支持可选 `=` 填充并严格校验非法字符与错误填充长度。

### 2. `Hotp` & `Totp`
- **`OtpAlgorithm`**: `.Sha1`、`.Sha256`、`.Sha512`。
- **`Hotp.generate(secret, counter, digits, alg)`**: RFC 4226 动态截断生成 1~9 位整数验证码；另有 `Hotp.formatCode`、`Hotp.generateCode` 与恒定时间 `Hotp.verify`。
- **`TotpConfig`**: `{ period_sec: u64 = 30, digits: u8 = 6, alg: OtpAlgorithm = .Sha1 }`。
- **`Totp.generateAt(secret, unix_sec, cfg)`** / **`Totp.generateCodeAt(...)`**: 基于给定 Unix 秒时间戳生成 TOTP 验证码。
- **`Totp.verifyAt(secret, code, unix_sec, skew_steps, cfg)`**: 在 `[step - skew_steps, step + skew_steps]` 窗口内遍历所有步长并采用 `constantTimeEql` 恒定时间比对（防前导零丢失与时序泄露）。
- **`Totp.buildOtpAuthUri(alloc, issuer, account, secret_b32, cfg)`**: 生成标准 `otpauth://totp/...` 二维码配对 URI，自动对 `issuer` 与 `account` 执行 RFC 3986 百分号编码，并校验控制字符注入与 Base32 密钥合法性。

---

## 四、 Fetch CORS 跨域策略引擎 (`cors.cp`)

- **`CorsConfig`**: 配置 `allowed_origins`、`allowed_methods`、`allowed_headers`、`exposed_headers`、`allow_credentials`、`max_age_sec`、`allow_any_origin`。
- **凭据安全守卫**: 当 `allow_credentials = true` 时，严格拒绝 `allow_any_origin = true` 或 `allowed_origins` 中包含 `"*"` 的不安全配置（返回 `CorsRejectReason.InvalidConfigCredentialsWithWildcard`, HTTP `403`）。
- **精确源与安全子域匹配**:
  - 严格校验证协议（`http`/`https`）、禁止 userinfo (`@`) / path / CRLF 注入与 `"null"` 源；
  - 支持 `"https://*.example.com"`（含可选端口）安全子域通配：精确匹配 `.example.com` 标签边界，彻底拦截 `evil-example.com`、`http://` 降级与 `evil.com@example.com` 绕过。
- **`CorsPolicy.evaluate(origin, method, req_headers)`**: 零堆分配评估简单请求与 `OPTIONS` 预检请求，生成完整响应状态码（预检 `204` / 简单 `200` / 拒绝 `403`）及 `Access-Control-*` 与 `Vary` 响应头集合。

---

## 五、 CSP 策略构造器与安全头预设 (`csp.cp`)

- **`CspBuilder`**:
  - 链式设置 `defaultSrc`、`scriptSrc`、`styleSrc`、`imgSrc`、`connectSrc`、`frameAncestors` 及通用 `directive(name, value)`；
  - 支持 `addNonce(directive, nonce)`（生成 `'nonce-...'`）与 `addSha256(directive, b64_hash)`（生成 `'sha256-...'`）；
  - **防注入校验**: 拦截 `\r`、`\n`、`\0` 与 `;` 走私字符；`build(alloc)` 在检测到非法输入时直接返回错误 `err.UnsafeCspDirective`。
- **`SecurityHeadersPreset`**:
  - `strictApi()`: 生成 API 严格响应头（`Strict-Transport-Security`、`X-Content-Type-Options: nosniff`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`Permissions-Policy`、`Content-Security-Policy`）。
  - `strictWeb()`: 生成 Web 页面严格响应头（`Strict-Transport-Security`、`X-Content-Type-Options: nosniff`、`X-Frame-Options: DENY`、`Referrer-Policy: strict-origin-when-cross-origin`、`Permissions-Policy`、`Content-Security-Policy`）。

---

## 六、 完整代码示例

```typescript
import {
    CSRF, TokenFormat, DEFAULT_EXPIRATION_MS,
    Base32, OtpAlgorithm, TotpConfig, Totp,
    CorsConfig, CorsPolicy,
    CspBuilder, SecurityHeadersPreset,
} from "cap:security";
const std = @import("std");

pub fn demoSecuritySuite(): !void {
    const alloc = std.heap.page_allocator;

    // 1. TOTP & otpauth URI
    const raw_secret = Base32.decode(alloc, "JBSWY3DPEHPK3PXP")?;
    defer alloc.free(raw_secret);
    const cfg = TotpConfig{ period_sec: 30, digits: 6, alg: .Sha1 };
    let code_buf: [10]u8 = undefined;
    const code = Totp.generateCodeAt(raw_secret, 1700000000, cfg, &code_buf)?;
    const ok = Totp.verifyAt(raw_secret, code, 1700000000, 1, cfg);

    // 2. CORS Policy
    const origins: [1][]const u8 = ["https://*.example.com"];
    const policy = CorsPolicy.init(CorsConfig{
        allowed_origins: &origins,
        allow_credentials: true,
    });
    const cors = policy.evaluate("https://app.example.com", "OPTIONS", []);

    // 3. CSP & Security Headers
    let csp = CspBuilder.init();
    * = csp.defaultSrc("'self'").scriptSrc("'self'").addNonce("script-src", "nonce123");
    const csp_str = csp.build(alloc)?;
    defer alloc.free(csp_str);
    const hdrs = SecurityHeadersPreset.strictApi();
}
```

---

## 上游 Bun `csrf` 行为契约与安全边界注意事项

1. **恒定时间签名比对（`constantTimeEql`）**：
   - 源自 Bun `src/csrf/csrf.cp`：验证 64 字节无状态 CSRF 令牌的 HMAC-SHA256 签名与用户提供的 TOTP 验证码时，必须通过 `constantTimeEql` 逐字节常量时间比较，杜绝基于响应耗时的侧信道泄露。
2. **CSRF 过期时间窗与时钟偏移**：
   - 令牌内置 `Timestamp (ms)` 与 `ExpiresIn (ms)`（默认 `DEFAULT_EXPIRATION_MS`），验证端除校验签名外须同时检查当前时间未超出有效期且未来自异常未来时间戳。
3. **CORS 凭据与通配源互斥（`CorsPolicy`）**：
   - 遵循 Fetch CORS 规范：当 `allow_credentials: true` 时，响应头 `Access-Control-Allow-Origin` **严禁**回显 `*`，必须回显经白名单（`allowed_origins`）校验通过的具体请求 `Origin` 并附带 `Vary: Origin`。
4. **TOTP 容错窗口限制（`Totp.verifyAt`）**：
   - 建议将时间步长容错窗口（`skew`）控制在 `±1`（30 秒步长）以内，并在服务端记录已消费时间步防止同一窗口内重放。
