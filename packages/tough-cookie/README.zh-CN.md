[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# tough-cookie

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`npm:tough-cookie`](https://github.com/salesforce/tough-cookie)（BSD-3-Clause）与 [`crates.io:cookie_store`](https://github.com/pfernie/cookie_store)（MIT）| CP 包版本：`1.0.0` | 依赖：`unicase`

零堆分配的 RFC 6265 HTTP `Cookie` / `Set-Cookie` 解析器、序列化器、域名/路径匹配器与定长 `CookieJar`。

## 零成本迁移对照（TypeScript / Rust → CP）

直接沿用原包名 `"tough-cookie"`，无需改名即可完成迁移：

| TypeScript (`npm:tough-cookie`) / Rust (`cookie_store`) | CP (`tough-cookie`) |
| :--- | :--- |
| `import { Cookie, CookieJar } from "tough-cookie";` | `import { Cookie, CookieJar, parseSetCookie } from "tough-cookie";` |
| `Cookie.parse(header)` | `parseSetCookie(header)` |
| `cookie.toString()` | `serializeSetCookie(cookie, out)` |
| `jar.setCookieSync(header, url)` | `jar.setFromHeader(header)` |
| `jar.getCookieStringSync(url)` | `jar.buildCookieHeader(host, path, is_https, out)` |

## 核心 API

### 导出常量

- `SAMESITE_NONE_UNSET: u8 = 0`
- `SAMESITE_LAX: u8 = 1`
- `SAMESITE_STRICT: u8 = 2`
- `SAMESITE_NONE: u8 = 3`
- `CJ_MAX_COOKIES: usize = 32`

### 类型与函数

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `Cookie { name, value, domain, path, max_age, has_max_age, secure, http_only, same_site }` | 零拷贝解析出的 Cookie 字段（字符串切片借用自输入请求头）。 |
| `parseSetCookie(header: string): Cookie` | 解析 RFC 6265 `Set-Cookie` 响应头。 |
| `serializeSetCookie(c: Cookie, out: []u8): usize` | 将 `Cookie` 序列化为 `Set-Cookie` 字节流写入 `out`。 |
| `getCookieHeaderValue(header: string, name: string): string?` | 从请求头 `Cookie: a=1; b=2` 中按键提取值。 |
| `domainMatches(host: string, cookie_domain: string): bool` | RFC 6265 §5.1.3 大小写不敏感域名后缀匹配。 |
| `pathMatches(req_path: string, cookie_path: string): bool` | RFC 6265 §5.1.4 路径前缀与斜杠边界匹配。 |
| `CookieJar.new(): CookieJar` | 创建空 32 槽位栈上 `CookieJar`。 |
| `jar.size(): usize` | 返回当前存储的 Cookie 数量。 |
| `jar.setFromHeader(*_, set_cookie_header: string): bool` | 解析并按 `name + domain + path` 写入或覆盖条目。 |
| `jar.buildCookieHeader(host, req_path, is_https, out: []u8): usize` | 按域名、路径与 HTTPS 策略拼装请求 `Cookie` 头写入 `out`。 |

## 实战示例

### 解析与序列化 `Set-Cookie`

```cp
import { parseSetCookie, serializeSetCookie, getCookieHeaderValue } from "tough-cookie";

fn demoParseSerialize() {
    const hdr = "sid=abc123; Domain=.example.com; Path=/api; Max-Age=3600; Secure; HttpOnly; SameSite=Strict";
    const c = parseSetCookie(hdr);
    // c.name == "sid", c.value == "abc123", c.secure == true, c.http_only == true

    let out: [256]u8 = undefined;
    const n = serializeSetCookie(c, out[0..]);
    * = n;

    // 从请求 Cookie 头提取指定键
    const tok = getCookieHeaderValue("theme=dark; sid=abc123", "sid");
    // tok.? == "abc123"
    * = tok;
}
```

### `CookieJar` 域名 / 路径 / HTTPS 策略过滤

```cp
import { CookieJar } from "tough-cookie";

fn demoJar() {
    let jar = CookieJar.new();
    * = jar.setFromHeader("sid=xyz; Domain=example.com; Path=/api; Secure");
    * = jar.setFromHeader("pref=zh; Domain=example.com; Path=/");

    let out: [256]u8 = undefined;
    // HTTP 请求：自动过滤 Secure Cookie
    const n_http = jar.buildCookieHeader("api.example.com", "/api/v1", false, out[0..]);
    // out[0..n_http] == "pref=zh"

    // HTTPS 请求：同时携带两条 Cookie
    const n_https = jar.buildCookieHeader("api.example.com", "/api/v1", true, out[0..]);
    // out[0..n_https] == "sid=xyz; pref=zh"
    * = n_http; * = n_https;
}
```

## 性能与安全亮点

- **零堆分配**：`CookieJar` 在栈上内联 `[32]Cookie = undefined`，无需堆分配且跳过初始化清零。
- **`@memcpy` 极速序列化**：`serializeSetCookie` 与 `buildCookieHeader` 直接通过 `@memcpy` 写入调用方缓冲区。
- **零分配大小写无关匹配**：复用 `unicase`（`eqAscii`、`endsWithIgnoreCase`）完成属性名与域名匹配，避免构造小写临时字符串。
- **RFC 6265 安全边界**：内置 `Secure` 协议校验、域名标签边界校验与路径分段校验。

## 许可协议

MIT
