# tough-cookie@1.0.0 (Agent Quickref)

> Zero-heap RFC 6265 HTTP Cookie, Set-Cookie parser, serializer, and domain/path matching jar (clean-room port of npm cookie + tough-cookie) | License: MIT
> Import: `import { ... } from "tough-cookie";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn parseSetCookie(header: string): Cookie;`
- `export fn serializeSetCookie(c: Cookie, out: []u8): usize;`
- `export fn getCookieHeaderValue(header: string, name: string): string?;`
- `export fn domainMatches(host: string, cookie_domain: string): bool;`
- `export fn pathMatches(req_path: string, cookie_path: string): bool;`
