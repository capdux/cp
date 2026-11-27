[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/pressure

Official overload protection, body limit (413), load shedding (503), and K8s /healthz /readyz probes for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/pressure` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/pressure
```

```cp
import {
    PressureConfig,
    pressureSetReady,
    pressureSetInflightForTest,
    pressureShedCount,
    pressureResetStats,
    pressStrEql,
    pressureOnRequest,
    pressureOnAfter,
} from "@web/pressure";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PressureConfig` | `class` | Fields: `max_body_bytes: usize, max_inflight: u32, retry_after_secs: string, health_path: string, ready_path: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pressureSetReady` | `(ready: bool)` | Executes `pressureSetReady` in-place. |
| `pressureSetInflightForTest` | `(n: u32)` | Executes `pressureSetInflightForTest` in-place. |
| `pressureShedCount` | `(): u64` | Invokes `pressureShedCount` and returns `u64`. |
| `pressureResetStats` | `()` | Executes `pressureResetStats` in-place. |
| `pressStrEql` | `(a: string, b: string): bool` | Invokes `pressStrEql` and returns `bool`. |
| `pressureOnRequest` | `(ctx: &mut Context): bool` | Invokes `pressureOnRequest` and returns `bool`. |
| `pressureOnAfter` | `(ctx: &mut Context, resp: &mut Response)` | Executes `pressureOnAfter` in-place. |
| `handleHealthz` | `(ctx: &mut Context): Response` | Invokes `handleHealthz` and returns `Response`. |
| `handleReadyz` | `(ctx: &mut Context): Response` | Invokes `handleReadyz` and returns `Response`. |
| `createPressurePlugin` | `(cfg: PressureConfig): Plugin` | Constructs/initializes an instance via `createPressurePlugin`. |
| `registerPressure` | `(app: &mut App, cfg: PressureConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { pressStrEql } from "@web/pressure";

pub fn main() {
    let result = pressStrEql("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
