# methods@1.0.0 (Agent Quickref)

> HTTP methods supported by RFC 9110 / WebDAV with safe/idempotent/cacheable predicates (clean-room CP implementation of npm methods) | License: MIT
> Import: `import { ... } from "methods";` | Full types: `src/index.d.cp`

## Functions
- `export fn hmToLower(c: u8): u8;`
- `export fn hmCmpLower(input: string, lower_target: string): i32;`
- `export fn findMethodIndex(method: string): i32;`
- `export fn isHttpMethod(method: string): bool;`
- `export fn toLowerMethod(method: string): string;`
- `export fn toUpperMethod(method: string): string;`
- `export fn methodAt(idx: usize): string;`
- `export fn isSafeMethod(method: string): bool;`
- `export fn isIdempotentMethod(method: string): bool;`
- `export fn isCacheableMethod(method: string): bool;`
- `export fn mayHaveRequestBody(method: string): bool;`
