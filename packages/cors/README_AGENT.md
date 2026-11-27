# cors — AI Agent Quick Ref

Zero-heap Fetch/W3C CORS origin evaluator and preflight/actual response header builder (clean-room port of npm cors) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CorsOptions,
    CorsDecision,
    corsPush,
    corsPushU32,
    corsIsWs,
    corsTrim,
    isOriginAllowed,
    isMethodAllowed,
} from "cors";
```

## Core Types & Constants

```cp
class CorsOptions { allow_any_origin: bool, reflect_origin: bool, allowed_origins: [16]string, allowed_origin_count: usize, /* +6 fields */ } // methods: permissive(...) +2
class CorsDecision { allowed: bool, is_preflight: bool, allow_origin: string, vary_origin: bool, /* +6 fields */ }
```

## API Signatures

```cp
fn corsPush(out: []u8, pos: usize, s: string): usize;
fn corsPushU32(out: []u8, pos: usize, v: u32): usize;
fn corsIsWs(b: u8): bool;
fn corsTrim(s: string): string;
fn isOriginAllowed(opts: CorsOptions, request_origin: string): bool;
fn isMethodAllowed(allowed_methods: string, method: string): bool;
fn evaluateCors(opts: CorsOptions, method: string, origin: string, ac_req_method: string, ac_req_headers: string): CorsDecision;
fn formatCorsHeaders(d: CorsDecision, out: []u8): usize;
```

## Minimal Usage

```cp
import { isOriginAllowed } from "cors";

pub fn main() {
    let result = isOriginAllowed(/* opts: CorsOptions */, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cors"`; install via `cap add cors`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
