# `cap:http_cookie`

纯 CP 零堆分配 RFC 6265 / 6265bis HTTP `Cookie` & `Set-Cookie` 解析、域/路径匹配与序列化库，吸收自 `/data/dev/bun/src/jsc/bindings/Cookie.cpp` 与 `CookieMap.cpp`。

## 导出符号

- `SameSite`：`unset | strict | lax | none`
- `Cookie`：`init(name, value)`、`parseSetCookie(header)`、`toSetCookieHeader(out)`、`matchesDomain(host)`、`matchesPath(req_path)`（支持 `Domain`、`Path`、`Expires`、`Max-Age`、`Secure`、`HttpOnly`、`Partitioned`、`SameSite`）
- `CookiePair` / `CookieRequestIterator` / `getCookie(cookie_header, name)`：零分配解析 `Cookie:` 请求头
