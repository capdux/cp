# cap:http_cookie_match

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 6265 §5.1.3/§5.1.4 与 RFC 6265bis 规范），提供纯 CP、零外部依赖、零堆分配的 Cookie 域名/路径匹配、默认路径计算、安全前缀校验与发送资格判定库。

## 导入方式

```cp
import {
    defaultCookiePath,
    cookieDomainMatches,
    cookiePathMatches,
    CookiePrefixKind,
    detectCookiePrefix,
    validateCookiePrefix,
    canSendCookie,
} from "cap:http_cookie_match";
```

## 核心 API

- `defaultCookiePath(request_uri_path): []const u8`：RFC 6265 §5.1.4 默认路径（`"/a/b/c"` -> `"/a/b"`，空串或单 `/` -> `"/"`）。
- `cookieDomainMatches(request_host, cookie_domain, host_only): bool`：RFC 6265 §5.1.3 域名匹配（支持前导 `.`、大小写无关；`host_only` 或 IPv4/IPv6 字面量强制精确匹配）。
- `cookiePathMatches(request_path, cookie_path): bool`：RFC 6265 §5.1.4 路径匹配（严格按 `/` 边界防前缀误匹配）。
- `CookiePrefixKind` / `detectCookiePrefix(name)` / `validateCookiePrefix(name, secure, domain_set, path)`：RFC 6265bis `__Secure-` 与 `__Host-` 约束校验。
- `canSendCookie(request_host, request_path, is_https, is_http_api, cookie_domain, host_only, cookie_path, secure_only, http_only): bool`。

## 使用示例

```cp
import { canSendCookie, validateCookiePrefix } from "cap:http_cookie_match";

fn example() {
    * = validateCookiePrefix("__Host-sid", true, false, "/"); // true
    * = canSendCookie("api.example.com", "/v1/users", true, true, ".example.com", false, "/v1", true, true); // true
}
```
