[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# cookie-signature

零堆分配的纯 CP HMAC-SHA256 Cookie 签名与常量时间验签（`sign` / `unsign`）、RFC 6265 `Cookie` 请求头解析（`getCookie`）以及 `Set-Cookie` 响应头格式化（`formatSetCookie`）库。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | npm |
| **上游原包** | `cookie-signature` (`tj/node-cookie-signature`) |
| **CP 包名** | `cookie-signature` (`v1.0.0`) |
| **许可证** | MIT |

CP 生态保持原包名 `cookie-signature` 不变，无需 `@cap/*` 前缀或改名：

```typescript
// TypeScript / Node.js (npm: cookie-signature)
import cookie from "cookie-signature";
const signed = cookie.sign("hello", "tobiiscool");
const val = cookie.unsign(signed, "tobiiscool"); // "hello" | false
```

```typescript
// CP（保持原包名 "cookie-signature"）
import { sign, unsign } from "cookie-signature";
let mut dst: [128]u8 = undefined;
let n = sign("hello", "tobiiscool", dst[0..128]);
let res = unsign(dst[0..n], "tobiiscool"); // res.ok == true, res.val_len == 5
```

## 安装

```bash
cap add cookie-signature
```

## 快速上手与实战示例

### 1. Cookie 签名与常量时间验签（`sign` / `unsign`）

```typescript
import { sign, unsign } from "cookie-signature";

fn verifySessionCookie(): void {
    let val = "session_abc123";
    let secret = "super-secret-key";

    // 输出格式："<val>.<43 字节 base64url HMAC-SHA256>"
    // 所需缓冲区长度：val.len + 1 + 43
    let mut signed_buf: [128]u8 = undefined;
    let written = sign(val, secret, signed_buf[0..128]);

    let res = unsign(signed_buf[0..written], secret);
    if (res.ok) {
        let verified_val = signed_buf[0..res.val_len]; // "session_abc123"
    }
}
```

### 2. 解析 `Cookie` 请求头与格式化 `Set-Cookie` 响应头

```typescript
import {
    getCookie,
    formatSetCookie,
    CookieOptions,
    SAME_SITE_LAX,
} from "cookie-signature";

fn cookieHeaders(): void {
    // 零拷贝解析 RFC 6265 Cookie 头
    let hdr = "theme=dark; sid=abc123; lang=en";
    let found = getCookie(hdr, "sid");
    if (found.found) {
        let sid = hdr[found.start .. found.start + found.len]; // "abc123"
    }

    // 格式化 Set-Cookie 响应头
    let opts = CookieOptions {
        max_age: 3600,
        path: "/",
        domain: "",
        http_only: true,
        secure: true,
        same_site: SAME_SITE_LAX,
    };
    let mut out: [256]u8 = undefined;
    let n = formatSetCookie("sid", "abc123", opts, out[0..256]);
    if (n > 0) {
        // out[0..(n as usize)] -> "sid=abc123; Max-Age=3600; Path=/; HttpOnly; Secure; SameSite=Lax"
    }
}
```

### 3. 内置 SHA-256、HMAC-SHA256 与常量时间比较

```typescript
import { sha256, hmacSha256, ctEqual, encodeBase64Url32 } from "cookie-signature";

fn cryptoPrimitives(): void {
    let mut digest: [32]u8 = undefined;
    sha256("hello", digest[0..32]);

    let mut mac: [32]u8 = undefined;
    hmacSha256("key", "message", mac[0..32]);

    let mut b64: [43]u8 = undefined;
    encodeBase64Url32(mac[0..32], b64[0..43]);

    let same = ctEqual(mac[0..32], mac[0..32]); // 常量时间防时序侧信道比较
}
```

## API 速览

### 常量
- `SAME_SITE_OMIT: u8`（`0`）、`SAME_SITE_STRICT: u8`（`1`）、`SAME_SITE_LAX: u8`（`2`）、`SAME_SITE_NONE: u8`（`3`）

### Cookie 签名与头部处理
- `UnsignResult { ok: bool, val_len: usize }`、`CookieSlice { start: usize, len: usize, found: bool }`
- `CookieOptions { max_age: i64, path: &[]u8, domain: &[]u8, http_only: bool, secure: bool, same_site: u8 }`
- `sign(val, secret, dst): usize`、`unsign(signed_val, secret): UnsignResult`
- `getCookie(header, name): CookieSlice`、`formatSetCookie(name, value, opts, dst): i64`

### 密码学与缓冲辅助函数
- `Sha256State`（`init`、`compress`、`update`、`finish`）、`sha256Init`、`sha256Update`、`sha256Final`、`sha256`
- `hmacSha256`、`ctEqual`、`encodeBase64Url32`、`appendBytes`、`appendU64`
