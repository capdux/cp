# @cap/cookie-jar

Clean-room MIT, zero-heap RFC 6265 HTTP Cookie & `Set-Cookie` parser, serializer, and domain/path matching `CookieJar` for CP.

## Dependencies
- `@cap/unicase` (`1.0.0`) — reused for case-insensitive attribute names and RFC 6265 domain suffix matching.

## API
- `parseSetCookie(header)` — parses `Set-Cookie` into `Cookie` (`name`, `value`, `domain`, `path`, `max_age`, `secure`, `http_only`, `same_site`)
- `serializeSetCookie(cookie, out)` — formats `Set-Cookie` header
- `getCookieHeaderValue(header, name)` — extracts value from `Cookie:` request header
- `domainMatches(host, cookie_domain)` & `pathMatches(req_path, cookie_path)`
- `CookieJar` (`new`, `size`, `setFromHeader`, `buildCookieHeader`)
