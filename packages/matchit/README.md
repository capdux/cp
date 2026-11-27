[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# matchit

Zero-allocation URL router with static, named (:id / {id}), and catch-all (*rest / {*rest}) priority matching (Rust matchit parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:matchit` ([https://github.com/ibraheemdev/matchit](https://github.com/ibraheemdev/matchit))

| Metadata | Value |
| --- | --- |
| Package | `matchit` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add matchit
```

```cp
import {
    SegKind,
    RouteParam,
    RouteMatch,
    RouteSeg,
    RouteEntry,
    Router,
    emptyParam,
    noMatch,
} from "matchit";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SegKind` | `Static, Param, CatchAll` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RouteParam` | `class` | Fields: `key: string, value: string` |
| `RouteMatch` | `class` | Fields: `matched: bool, handler_id: u32, params: [MAX_PARAMS]RouteParam, param_count: usize` · Methods: `param` |
| `RouteSeg` | `class` | Fields: `kind: SegKind, text: string` |
| `RouteEntry` | `class` | Fields: `active: bool, handler_id: u32, segs: [MAX_SEGS]RouteSeg, seg_count: usize` |
| `Router` | `class` | Fields: `routes: [MAX_ROUTES]RouteEntry, count: usize` · Methods: `init, insert, at, routeCount` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `emptyParam` | `(): RouteParam` | Invokes `emptyParam` and returns `RouteParam`. |
| `noMatch` | `(): RouteMatch` | Invokes `noMatch` and returns `RouteMatch`. |
| `emptySeg` | `(): RouteSeg` | Invokes `emptySeg` and returns `RouteSeg`. |
| `emptyEntry` | `(): RouteEntry` | Invokes `emptyEntry` and returns `RouteEntry`. |
| `segPriority` | `(kind: SegKind): u8` | Invokes `segPriority` and returns `u8`. |
| `parseRoutePattern` | `(pattern: string, handler_id: u32): RouteEntry?` | Parses input via `parseRoutePattern` and returns `RouteEntry?`. |
| `routesConflict` | `(a: RouteEntry, b: RouteEntry): bool` | Invokes `routesConflict` and returns `bool`. |
| `routeBetterThan` | `(cand: RouteEntry, best: RouteEntry): bool` | Invokes `routeBetterThan` and returns `bool`. |
| `matchEntry` | `(entry: RouteEntry, path: string): RouteMatch?` | Invokes `matchEntry` and returns `RouteMatch?`. |

## Minimal Example

```cp
import { parseRoutePattern } from "matchit";

pub fn main() {
    let result = parseRoutePattern("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
