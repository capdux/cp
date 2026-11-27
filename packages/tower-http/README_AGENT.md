# tower-http — AI Agent Quick Ref

Zero-heap CORS origin matcher, wildcard subdomain check, and preflight method/header validator for CP (compatible with npm cors & Rust tower-http::cors) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    findSchemeSep,
    isValidOrigin,
    matchOrigin,
    matchOriginList,
    isMethodAllowed,
    areHeadersAllowed,
    isSimpleMethod,
} from "tower-http";
```

## API Signatures

```cp
fn findSchemeSep(s: string): i64;
fn isValidOrigin(origin: string): bool;
fn matchOrigin(origin: string, pattern: string, allow_credentials: bool): bool;
fn matchOriginList(origin: string, patterns: &[]string, allow_credentials: bool): bool;
fn isMethodAllowed(method: string, allowed_methods: string): bool;
fn areHeadersAllowed(req_headers: string, allowed_headers: string): bool;
fn isSimpleMethod(method: string): bool;
```

## Minimal Usage

```cp
import { isValidOrigin } from "tower-http";

pub fn main() {
    let result = isValidOrigin("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tower-http"`; install via `cap add tower-http`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add tower-http
cpc check src/main.cp
```
