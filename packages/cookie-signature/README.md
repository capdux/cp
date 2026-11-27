[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# cookie-signature

Zero-heap HMAC-SHA256 cookie signing & constant-time verification (`sign` / `unsign`), RFC 6265 `Cookie` header parsing (`getCookie`), and `Set-Cookie` header formatting (`formatSetCookie`) in pure CP.

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | npm |
| **Upstream Package** | `cookie-signature` (`tj/node-cookie-signature`) |
| **CP Package** | `cookie-signature` (`v1.0.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```typescript
// TypeScript / Node.js (npm: cookie-signature)
import cookie from "cookie-signature";
const signed = cookie.sign("hello", "tobiiscool");
const val = cookie.unsign(signed, "tobiiscool"); // "hello" | false
```

```typescript
// CP (same package name: "cookie-signature")
import { sign, unsign } from "cookie-signature";
let mut dst: [128]u8 = undefined;
let n = sign("hello", "tobiiscool", dst[0..128]);
let res = unsign(dst[0..n], "tobiiscool"); // res.ok == true, res.val_len == 5
```

## Installation

```bash
cap add cookie-signature
```

## Quick Start & Practical Examples

### 1. Sign & Constant-Time Unsign (`sign` / `unsign`)

```typescript
import { sign, unsign } from "cookie-signature";

fn verifySessionCookie(): void {
    let val = "session_abc123";
    let secret = "super-secret-key";

    // Output format: "<val>.<43-char base64url HMAC-SHA256>"
    // Required buffer size: val.len + 1 + 43
    let mut signed_buf: [128]u8 = undefined;
    let written = sign(val, secret, signed_buf[0..128]);

    let res = unsign(signed_buf[0..written], secret);
    if (res.ok) {
        let verified_val = signed_buf[0..res.val_len]; // "session_abc123"
    }
}
```

### 2. Parse `Cookie` Header & Format `Set-Cookie` Header

```typescript
import {
    getCookie,
    formatSetCookie,
    CookieOptions,
    SAME_SITE_LAX,
} from "cookie-signature";

fn cookieHeaders(): void {
    // Parse RFC 6265 Cookie request header (zero-copy slice indices)
    let hdr = "theme=dark; sid=abc123; lang=en";
    let found = getCookie(hdr, "sid");
    if (found.found) {
        let sid = hdr[found.start .. found.start + found.len]; // "abc123"
    }

    // Format Set-Cookie response header
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

### 3. Built-in SHA-256, HMAC-SHA256 & Constant-Time Comparison

```typescript
import { sha256, hmacSha256, ctEqual, encodeBase64Url32 } from "cookie-signature";

fn cryptoPrimitives(): void {
    let mut digest: [32]u8 = undefined;
    sha256("hello", digest[0..32]);

    let mut mac: [32]u8 = undefined;
    hmacSha256("key", "message", mac[0..32]);

    let mut b64: [43]u8 = undefined;
    encodeBase64Url32(mac[0..32], b64[0..43]);

    let same = ctEqual(mac[0..32], mac[0..32]); // constant-time equality
}
```

## API Reference

### Constants
- `SAME_SITE_OMIT: u8` (`0`), `SAME_SITE_STRICT: u8` (`1`), `SAME_SITE_LAX: u8` (`2`), `SAME_SITE_NONE: u8` (`3`)

### Cookie Signing & Headers
- `UnsignResult { ok: bool, val_len: usize }`
- `CookieSlice { start: usize, len: usize, found: bool }`
- `CookieOptions { max_age: i64, path: &[]u8, domain: &[]u8, http_only: bool, secure: bool, same_site: u8 }`
- `sign(val: &[]u8, secret: &[]u8, dst: []u8): usize`
- `unsign(signed_val: &[]u8, secret: &[]u8): UnsignResult`
- `getCookie(header: &[]u8, name: &[]u8): CookieSlice`
- `formatSetCookie(name: &[]u8, value: &[]u8, opts: CookieOptions, dst: []u8): i64`

### Cryptographic & Buffer Helpers
- `Sha256State` (`init()`, `compress(blk)`, `update(data)`, `finish(out)`), `sha256Init()`, `sha256Update()`, `sha256Final()`, `sha256(data, out32)`
- `hmacSha256(key, msg, out)`, `ctEqual(a, b): bool`, `encodeBase64Url32(mac, dst)`
- `appendBytes(dst, pos, src): i64`, `appendU64(dst, pos, val): i64`
