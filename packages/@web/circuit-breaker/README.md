[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/circuit-breaker

Official Envoy / Tower three-state circuit breaker (Closed / Open / HalfOpen) plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/circuit-breaker` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/circuit-breaker
```

```cp
import {
    CircuitState,
    CircuitBreakerConfig,
    cbNowSec,
    cbState,
    cbFailureCount,
    cbSetTestClock,
    cbReset,
    cbBeforeHandle,
} from "@web/circuit-breaker";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `CircuitState` | `Closed, Open, HalfOpen` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CircuitBreakerConfig` | `class` | Fields: `failure_threshold: u32, reset_timeout_secs: u64, retry_after_secs: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cbNowSec` | `(): u64` | Invokes `cbNowSec` and returns `u64`. |
| `cbState` | `(): CircuitState` | Invokes `cbState` and returns `CircuitState`. |
| `cbFailureCount` | `(): u32` | Invokes `cbFailureCount` and returns `u32`. |
| `cbSetTestClock` | `(now_sec: u64)` | Executes `cbSetTestClock` in-place. |
| `cbReset` | `()` | Executes `cbReset` in-place. |
| `cbBeforeHandle` | `(ctx: &mut Context): bool` | Invokes `cbBeforeHandle` and returns `bool`. |
| `cbAfterHandle` | `(ctx: &mut Context, resp: &mut Response)` | Executes `cbAfterHandle` in-place. |
| `cbCreatePlugin` | `(cfg: CircuitBreakerConfig): Plugin` | Invokes `cbCreatePlugin` and returns `Plugin`. |
| `registerCircuitBreaker` | `(app: &mut App, cfg: CircuitBreakerConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { cbNowSec } from "@web/circuit-breaker";

pub fn main() {
    let result = cbNowSec();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
