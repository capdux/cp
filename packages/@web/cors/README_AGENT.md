# @web/cors — AI Agent Quick Ref

Official CORS middleware plugin for cap:web (preflight OPTIONS, origin allowlist, credentials, exposed headers) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CorsConfig,
    corsHasCtlOrCrlf,
    corsBytesEql,
    isOriginAllowed,
    applyCorsHeaders,
    corsOnRequest,
    createCorsPlugin,
    registerCors,
} from "@web/cors";
```

## Core Types & Constants

```cp
class CorsConfig { allow_origin: string, allow_methods: string, allow_headers: string, expose_headers: string, /* +2 fields */ }
```

## API Signatures

```cp
fn corsHasCtlOrCrlf(s: string): bool;
fn corsBytesEql(a: string, b: string): bool;
fn isOriginAllowed(allow_origin: string, origin: string): bool;
fn applyCorsHeaders(ctx: &mut Context, origin: string);
fn corsOnRequest(ctx: &mut Context): bool;
fn createCorsPlugin(cfg: CorsConfig): Plugin;
fn registerCors(app: &mut App, cfg: CorsConfig): &mut App;
```

## Minimal Usage

```cp
import { isOriginAllowed } from "@web/cors";

pub fn main() {
    let result = isOriginAllowed("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/cors"`; install via `cap add @web/cors`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
