# matchit — AI Agent Quick Ref

Zero-allocation URL router with static, named (:id / {id}), and catch-all (*rest / {*rest}) priority matching (Rust matchit parity) for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum SegKind { Static, Param, CatchAll }
class RouteParam { key: string, value: string }
class RouteMatch { matched: bool, handler_id: u32, params: [MAX_PARAMS]RouteParam, param_count: usize } // methods: param(...)
class RouteSeg { kind: SegKind, text: string }
class RouteEntry { active: bool, handler_id: u32, segs: [MAX_SEGS]RouteSeg, seg_count: usize }
class Router { routes: [MAX_ROUTES]RouteEntry, count: usize } // methods: init(...) +3
```

## API Signatures

```cp
fn emptyParam(): RouteParam;
fn noMatch(): RouteMatch;
fn emptySeg(): RouteSeg;
fn emptyEntry(): RouteEntry;
fn segPriority(kind: SegKind): u8;
fn parseRoutePattern(pattern: string, handler_id: u32): RouteEntry?;
fn routesConflict(a: RouteEntry, b: RouteEntry): bool;
fn routeBetterThan(cand: RouteEntry, best: RouteEntry): bool;
fn matchEntry(entry: RouteEntry, path: string): RouteMatch?;
```

## Minimal Usage

```cp
import { parseRoutePattern } from "matchit";

pub fn main() {
    let result = parseRoutePattern("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"matchit"`; install via `cap add matchit`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
