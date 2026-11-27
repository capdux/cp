# cookie-parser@1.0.0 (Agent Quickref)

> Zero-heap HTTP Cookie header parser with HMAC-SHA256 signed cookie verification (s:), key rotation, and JSON cookie (j:) extraction (npm cookie-parser equivalent) | License: MIT
> Import: `import { ... } from "cookie-parser";` | Full types: `src/index.d.cp`
> Deps: `cookie-signature`, `tough-cookie`

## Functions
- `export fn cprStrEq(a: string, b: string): bool;`
- `export fn cprStartsWith2(s: string, c0: u8, c1: u8): bool;`
- `export fn isSignedCookie(val: string): bool;`
- `export fn isJsonCookie(val: string): bool;`
- `export fn stripJsonCookiePrefix(val: string): string?;`
- `export fn signCookieValue(val: string, secret: string, out: []u8): usize;`
- `export fn signedCookie(val: string, secret: string): string?;`
- `export fn signedCookieWithSecrets(val: string, secrets: [4]string, secret_count: usize): string?;`
- `export fn lookupRawCookie(header: string, name: string): string?;`
- `export fn cprParseInto(header: string, secrets: [4]string, secret_count: usize): ParsedCookies;`
- `export fn parseCookieHeader(header: string, secret: string): ParsedCookies;`
- `export fn parseCookieHeaderWithSecrets(header: string, secrets: [4]string, secret_count: usize): ParsedCookies;`
