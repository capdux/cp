# @web/security — AI Agent Quick Ref

Official Helmet security headers (HSTS/CSP/X-Frame-Options/Referrer-Policy) and Double-Submit CSRF plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SecurityConfig,
    constantTimeEql,
    isSafeHttpMethod,
    securityOnRequest,
    createSecurityPlugin,
    registerSecurity,
} from "@web/security";
```

## Core Types & Constants

```cp
class SecurityConfig { hsts: string, csp: string, x_frame_options: string, x_content_type_options: string, /* +6 fields */ }
```

## API Signatures

```cp
fn constantTimeEql(a: string, b: string): bool;
fn isSafeHttpMethod(m: AppMethod): bool;
fn securityOnRequest(ctx: &mut Context): bool;
fn createSecurityPlugin(cfg: SecurityConfig): Plugin;
fn registerSecurity(app: &mut App, cfg: SecurityConfig): &mut App;
```

## Minimal Usage

```cp
import { constantTimeEql } from "@web/security";

pub fn main() {
    let result = constantTimeEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/security"`; install via `cap add @web/security`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
