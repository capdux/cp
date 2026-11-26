# @cap/cookie-jar

Zero-allocation RFC 6265 HTTP Cookie domain/path matcher, `Set-Cookie` parser, and `Cookie` header builder for CP.

## Features

- **Zero heap allocation**: zero-copy `CookieEntry` fields and caller-supplied `[]u8` output buffers.
- **RFC 6265 matching**: `domainMatches` (dot-boundary suffix check blocking spoofed domains like `evil-example.com`), `pathMatches`, and `defaultPath`.
- **Full attribute parsing**: `parseSetCookie` handles `Domain`, `Path`, `Max-Age`, `Secure`, `HttpOnly`, and `SameSite` (`None`/`Lax`/`Strict`).
- **Safe request assembly**: `shouldSend` enforces HTTPS for `Secure` cookies and drops expired (`Max-Age <= 0`) cookies; `buildCookieHeader` assembles `k1=v1; k2=v2` without partial truncation.

## Usage

```cp
import { CookieEntry, parseSetCookie, buildCookieHeader } from "@cap/cookie-jar";

let c = CookieEntry{ name: "", value: "", domain: "", path: "", max_age: 0, has_max_age: false, secure: false, http_only: false, same_site: 0 };
* = parseSetCookie("sid=xyz; Domain=.example.com; Path=/api; Secure; HttpOnly; SameSite=Strict", "api.example.com", "/api/login", &c);
```
