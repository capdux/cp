# http@1.0.0 (Agent Quickref)

> Zero-allocation HTTP Method, StatusCode, Version, and HeaderName core types (crates.io http v1 parity) composing @cap/unicase for CP. | License: MIT
> Import: `import { ... } from "http";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn parseMethod(s: string): u8;`
- `export fn methodAsStr(m: u8): string;`
- `export fn isSafeMethod(m: u8): bool;`
- `export fn isIdempotentMethod(m: u8): bool;`
- `export fn parseHttpVersion(s: string): u8;`
- `export fn httpVersionAsStr(v: u8): string;`
- `export fn isValidHeaderName(name: string): bool;`
- `export fn headerNameEq(a: string, b: string): bool;`
