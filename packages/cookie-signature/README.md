<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`cookie-signature`](https://github.com/tj/node-cookie-signature)（`MIT`）（原库名 `cookie-signature`，CP 包名 `@cap/cookie-sign`）。

# @cap/cookie-sign

Zero-heap RFC 6265 Cookie parser, `Set-Cookie` formatter, and built-in HMAC-SHA256 constant-time cookie signing/verification in pure CP (zero external dependencies).

## Features

- `getCookie(header, name)`: zero-copy cookie lookup with whitespace trimming and optional double-quote stripping
- `sign(val, secret, dst)` & `unsign(signed_val, secret)`: built-in FIPS 180-4 SHA-256 + RFC 2104 HMAC-SHA256 + URL-safe Base64 (no padding) with constant-time verification
- `formatSetCookie(name, value, opts, dst)`: RFC 6265 `Set-Cookie` header formatting (`Max-Age`, `Domain`, `Path`, `HttpOnly`, `Secure`, `SameSite`)

## Usage

```cp
import { sign, unsign, getCookie } from "@cap/cookie-sign";

let buf: [128]u8 = undefined;
const n = sign("uid=42", "my-secret", buf[0..128]);
const verified = unsign(buf[0..(n as usize)], "my-secret");
```
