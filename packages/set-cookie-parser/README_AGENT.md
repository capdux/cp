# set-cookie-parser@2.7.1 (Agent Quickref)

> Zero-allocation RFC 6265 Set-Cookie HTTP header parser and splitter | License: MIT
> Import: `import { ... } from "set-cookie-parser";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseSetCookie(header: string): ParsedCookie;`
- `export fn splitCookiesCount(combined: string): usize;`
- `export fn splitCookiesGet(combined: string, target_idx: usize): string?;`
