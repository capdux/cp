# parseurl@1.0.0 (Agent Quickref)

> Zero-allocation high-speed HTTP request URL pathname/query zero-copy splitter and path safety checker for CP (clean-room parity with Express parseurl) | License: MIT
> Import: `import { ... } from "parseurl";` | Full types: `src/index.d.cp`
> Deps: `querystring`

## Functions
- `export fn parseRequestUrl(raw_url: string): ParsedRequestUrl;`
- `export fn decodePathname(pathname: string, out: []u8): usize;`
- `export fn isSafeRequestPath(pathname: string): bool;`
- `export fn getRequestQueryParam(raw_url: string, key: string, out: []u8): usize?;`
- `export fn decodeQueryValue(raw: string, out: []u8): usize;`
