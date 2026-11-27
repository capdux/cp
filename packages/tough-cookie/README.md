[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# tough-cookie

> **Source Attribution**: Clean-room MIT CP implementation of [`npm:tough-cookie`](https://github.com/salesforce/tough-cookie) (BSD-3-Clause) & [`crates.io:cookie_store`](https://github.com/pfernie/cookie_store) (MIT) | CP Package Version: `1.0.0` | Dependency: `unicase`

Zero-heap RFC 6265 HTTP `Cookie` / `Set-Cookie` parser, serializer, domain/path matcher, and fixed-capacity `CookieJar` for CP.

## Zero-Cost Migration (TypeScript / Rust → CP)

Keep the exact same package name (`"tough-cookie"`) — no renaming required.

| TypeScript (`npm:tough-cookie`) / Rust (`cookie_store`) | CP (`tough-cookie`) |
| :--- | :--- |
| `import { Cookie, CookieJar } from "tough-cookie";` | `import { Cookie, CookieJar, parseSetCookie } from "tough-cookie";` |
| `Cookie.parse(header)` | `parseSetCookie(header)` |
| `cookie.toString()` | `serializeSetCookie(cookie, out)` |
| `jar.setCookieSync(header, url)` | `jar.setFromHeader(header)` |
| `jar.getCookieStringSync(url)` | `jar.buildCookieHeader(host, path, is_https, out)` |

## API Reference

### Constants

- `SAMESITE_NONE_UNSET: u8 = 0`
- `SAMESITE_LAX: u8 = 1`
- `SAMESITE_STRICT: u8 = 2`
- `SAMESITE_NONE: u8 = 3`
- `CJ_MAX_COOKIES: usize = 32`

### Types & Functions

| Symbol / Signature | Description |
| :--- | :--- |
| `Cookie { name, value, domain, path, max_age, has_max_age, secure, http_only, same_site }` | Zero-copy parsed cookie fields (slices borrow from input header). |
| `parseSetCookie(header: string): Cookie` | Parse an RFC 6265 `Set-Cookie` header value. |
| `serializeSetCookie(c: Cookie, out: []u8): usize` | Serialize `Cookie` to `Set-Cookie` header bytes into `out`. |
| `getCookieHeaderValue(header: string, name: string): string?` | Extract a named value from a request `Cookie: a=1; b=2` header. |
| `domainMatches(host: string, cookie_domain: string): bool` | RFC 6265 §5.1.3 case-insensitive domain suffix matching. |
| `pathMatches(req_path: string, cookie_path: string): bool` | RFC 6265 §5.1.4 path prefix & boundary matching. |
| `CookieJar.new(): CookieJar` | Create an empty 32-slot stack `CookieJar`. |
| `jar.size(): usize` | Number of cookies currently stored. |
| `jar.setFromHeader(*_, set_cookie_header: string): bool` | Parse and upsert (`name` + `domain` + `path`) a `Set-Cookie` header. |
| `jar.buildCookieHeader(host, req_path, is_https, out: []u8): usize` | Build `Cookie` request header for matching cookies into `out`. |

## Usage Examples

### Parsing & Serializing `Set-Cookie`

```cp
import { parseSetCookie, serializeSetCookie, getCookieHeaderValue } from "tough-cookie";

fn demoParseSerialize() {
    const hdr = "sid=abc123; Domain=.example.com; Path=/api; Max-Age=3600; Secure; HttpOnly; SameSite=Strict";
    const c = parseSetCookie(hdr);
    // c.name == "sid", c.value == "abc123", c.secure == true, c.http_only == true

    let out: [256]u8 = undefined;
    const n = serializeSetCookie(c, out[0..]);
    * = n;

    // Extract single cookie from request header
    const tok = getCookieHeaderValue("theme=dark; sid=abc123", "sid");
    // tok.? == "abc123"
    * = tok;
}
```

### `CookieJar` Domain / Path / HTTPS Filtering

```cp
import { CookieJar } from "tough-cookie";

fn demoJar() {
    let jar = CookieJar.new();
    * = jar.setFromHeader("sid=xyz; Domain=example.com; Path=/api; Secure");
    * = jar.setFromHeader("pref=zh; Domain=example.com; Path=/");

    let out: [256]u8 = undefined;
    // HTTP request: "sid" is excluded because Secure=true
    const n_http = jar.buildCookieHeader("api.example.com", "/api/v1", false, out[0..]);
    // out[0..n_http] == "pref=zh"

    // HTTPS request: both cookies included
    const n_https = jar.buildCookieHeader("api.example.com", "/api/v1", true, out[0..]);
    // out[0..n_https] == "sid=xyz; pref=zh"
    * = n_http; * = n_https;
}
```

## Performance & Safety Highlights

- **Zero Heap Allocation**: `CookieJar` stores `[32]Cookie` inline on the stack (`let arr: [32]Cookie = undefined`) with zero startup zeroing overhead.
- **Fast `@memcpy` Serialization**: Header serialization and `Cookie` assembly copy slices with `@memcpy` directly into caller buffers.
- **Case-Insensitive ASCII Matching**: Reuses `unicase` (`eqAscii`, `endsWithIgnoreCase`) for RFC 6265 attribute and domain matching without allocating lowercased copies.
- **RFC 6265 Security Enforcement**: `buildCookieHeader` automatically enforces `Secure` (HTTPS-only), domain label boundaries, and path segment boundaries.

## License

MIT
