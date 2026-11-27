[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# onetime

Ensure an initialization or action runs only once with call-count tracking (clean-room CP implementation of npm onetime)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:onetime` ([https://github.com/sindresorhus/onetime](https://github.com/sindresorhus/onetime))

| Metadata | Value |
| --- | --- |
| Package | `onetime` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add onetime
```

```cp
import { OnceCallResult, OnceGuard, OnceRegistry } from "onetime";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OnceCallResult` | `class` | Fields: `status: i32, executed_now: bool, value: i64, call_count: u64` |
| `OnceGuard` | `class` | Fields: `called: bool, strict: bool, call_count: u64, cached_value: i64` · Methods: `new, newStrict, isCalled, callCount, cachedValue, runWithValue, +2 more` |
| `OnceRegistry` | `class` | Fields: `keys: [64]u64, values: [64]i64, counts: [64]u64, len: usize` · Methods: `new, size, has, callCountOf, runForKey` |

## Minimal Example

```cp
import { OnceCallResult } from "onetime";

pub fn main() {
    // Refer to src/index.d.cp for OnceCallResult fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
