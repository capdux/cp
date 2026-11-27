# tough-cookie@1.0.0 — AI Agent Quick Ref

- **Source**: `npm:tough-cookie` (BSD-3-Clause) + `crates.io:cookie_store` (MIT) | **Deps**: `unicase`
- **Import**: `import { Cookie, CookieJar, parseSetCookie, serializeSetCookie, getCookieHeaderValue, domainMatches, pathMatches, SAMESITE_NONE_UNSET, SAMESITE_LAX, SAMESITE_STRICT, SAMESITE_NONE, CJ_MAX_COOKIES } from "tough-cookie";`
- **Full Types**: `packages/tough-cookie/src/index.d.cp`

## Constants
```cp
SAMESITE_NONE_UNSET: u8 = 0
SAMESITE_LAX:        u8 = 1
SAMESITE_STRICT:     u8 = 2
SAMESITE_NONE:       u8 = 3
CJ_MAX_COOKIES:   usize = 32
```

## Core Types & Functions
```cp
class Cookie {
    name: string, value: string, domain: string, path: string,
    max_age: i64, has_max_age: bool, secure: bool, http_only: bool, same_site: u8,
}
class CookieJar {
    items: [32]Cookie, count: usize,
    fn new(): CookieJar
    fn size(): usize
    fn setFromHeader(_: auto, set_cookie_header: string): bool
    fn buildCookieHeader(host: string, req_path: string, is_https: bool, out: []u8): usize
}

fn parseSetCookie(header: string): Cookie
fn serializeSetCookie(c: Cookie, out: []u8): usize
fn getCookieHeaderValue(header: string, name: string): string?
fn domainMatches(host: string, cookie_domain: string): bool
fn pathMatches(req_path: string, cookie_path: string): bool
```

## Minimal Pattern
```cp
import { CookieJar } from "tough-cookie";

let jar = CookieJar.new();
* = jar.setFromHeader("sid=abc; Domain=example.com; Path=/; Secure");
let out: [256]u8 = undefined;
const n = jar.buildCookieHeader("api.example.com", "/v1", true, out[0..]); // "sid=abc"
```

## Pitfalls & Notes
1. **Zero-Copy Slices**: `Cookie` fields (`name`, `value`, `domain`, `path`) borrow directly from the `header` string — keep `header` alive while using the `Cookie` or `CookieJar`.
2. **Capacity**: `CookieJar` holds up to `CJ_MAX_COOKIES` (32) cookies; `setFromHeader` returns `false` when full (or if `name` is empty).
3. **Upsert Semantics**: `setFromHeader` replaces an existing cookie with the same `(name, domain, path)`.
4. **Secure Flag**: `buildCookieHeader` omits `c.secure == true` cookies whenever `is_https == false`.
5. **Empty Domain**: If `cookie_domain == ""`, `domainMatches` returns `true` (host-only wildcard in this lightweight jar); set explicit `Domain=` when scoping across hosts.
