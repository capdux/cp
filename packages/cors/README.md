[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cors

Zero-heap Fetch/W3C CORS origin evaluator and preflight/actual response header builder (clean-room port of npm cors)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cors` ([https://github.com/expressjs/cors](https://github.com/expressjs/cors))（原库名 `cors` → CP 包名 `cors`）

| Metadata | Value |
| --- | --- |
| Package | `cors` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cors
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CorsOptions` | `class` | Fields: `allow_any_origin: bool, reflect_origin: bool, allowed_origins: [16]string, allowed_origin_count: usize, allowed_methods: string, allowed_headers: string, +4 more` · Methods: `permissive, restrictive, addOrigin` |
| `CorsDecision` | `class` | Fields: `allowed: bool, is_preflight: bool, allow_origin: string, vary_origin: bool, allow_credentials: bool, allow_methods: string, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `corsPush` | `(out: []u8, pos: usize, s: string): usize` | Invokes `corsPush` and returns `usize`. |
| `corsPushU32` | `(out: []u8, pos: usize, v: u32): usize` | Invokes `corsPushU32` and returns `usize`. |
| `corsIsWs` | `(b: u8): bool` | Invokes `corsIsWs` and returns `bool`. |
| `corsTrim` | `(s: string): string` | Invokes `corsTrim` and returns `string`. |
| `isOriginAllowed` | `(opts: CorsOptions, request_origin: string): bool` | Checks predicate `isOriginAllowed` and returns `bool`. |
| `isMethodAllowed` | `(allowed_methods: string, method: string): bool` | Checks predicate `isMethodAllowed` and returns `bool`. |
| `evaluateCors` | `(opts: CorsOptions, method: string, origin: string, ac_req_method: string, ac_req_headers: string): CorsDecision` | Invokes `evaluateCors` and returns `CorsDecision`. |
| `formatCorsHeaders` | `(d: CorsDecision, out: []u8): usize` | Serializes/formats data via `formatCorsHeaders` returning `usize`. |

## Minimal Example

```cp
import { isOriginAllowed } from "cors";

pub fn main() {
    let result = isOriginAllowed(/* opts: CorsOptions */, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
