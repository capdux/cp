[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# p-locate

Zero-allocation bounded-concurrency ordered/unordered locator planner (npm p-locate v7 equivalent) for CP, reusing p-limit.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-locate` ([https://github.com/sindresorhus/p-locate](https://github.com/sindresorhus/p-locate))

| Metadata | Value |
| --- | --- |
| Package | `p-locate` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add p-locate
```

```cp
import { PLocateOptions, LocateCandidate, PLocatePlanner, plocEmptyCandidate } from "p-locate";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PLocateOptions` | `class` | Fields: `concurrency: usize, preserve_order: bool` · Methods: `defaults` |
| `LocateCandidate` | `class` | Fields: `id: u32, matched: bool, finished: bool` |
| `PLocatePlanner` | `class` | Fields: `opts: PLocateOptions, limiter: PLimit, items: [32]LocateCandidate, count: usize, settled: bool, has_winner: bool, +1 more` · Methods: `new, defaultPlanner, addCandidate, dispatchNext, reportResult, isSettled, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `plocEmptyCandidate` | `(): LocateCandidate` | Invokes `plocEmptyCandidate` and returns `LocateCandidate`. |

## Minimal Example

```cp
import { plocEmptyCandidate } from "p-locate";

pub fn main() {
    let result = plocEmptyCandidate();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
