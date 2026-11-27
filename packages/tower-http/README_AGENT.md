# tower-http@1.0.0 (Agent Quickref)

> Zero-heap CORS origin matcher, wildcard subdomain check, and preflight method/header validator for CP (compatible with npm cors & Rust tower-http::cors) | License: MIT
> Import: `import { ... } from "tower-http";` | Full types: `src/index.d.cp`

## Functions
- `export fn findSchemeSep(s: string): i64;`
- `export fn isValidOrigin(origin: string): bool;`
- `export fn matchOrigin(origin: string, pattern: string, allow_credentials: bool): bool;`
- `export fn matchOriginList(origin: string, patterns: &[]string, allow_credentials: bool): bool;`
- `export fn isMethodAllowed(method: string, allowed_methods: string): bool;`
- `export fn areHeadersAllowed(req_headers: string, allowed_headers: string): bool;`
- `export fn isSimpleMethod(method: string): bool;`
