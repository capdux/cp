[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# p-timeout

Zero-allocation deadline tracker and timeout state machine guard (npm p-timeout v6 equivalent) for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-timeout` ([https://github.com/sindresorhus/p-timeout](https://github.com/sindresorhus/p-timeout))（原库名 `p-timeout` → CP 包名 `p-timeout`）

| Metadata | Value |
| --- | --- |
| Package | `p-timeout` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add p-timeout
```

```cp
import { TimeoutOptions, TimeoutGuard, ptoMaxU64, ptoSaturatingAdd } from "p-timeout";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TimeoutOptions` | `class` | Fields: `milliseconds: u64, has_fallback: bool, fallback_code: i32, error_message: string` · Methods: `withMs` |
| `TimeoutGuard` | `class` | Fields: `start_ms: u64, deadline_ms: u64, opts: TimeoutOptions, state: u8, result_code: i32, used_fallback: bool` · Methods: `new, deadlineMs, remainingMs, isExpired, poll, resolve, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ptoMaxU64` | `(): u64` | Invokes `ptoMaxU64` and returns `u64`. |
| `ptoSaturatingAdd` | `(a: u64, b: u64): u64` | Invokes `ptoSaturatingAdd` and returns `u64`. |

## Minimal Example

```cp
import { ptoMaxU64 } from "p-timeout";

pub fn main() {
    let result = ptoMaxU64();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
