# disjoint-sets — AI Agent Quick Ref

Zero-heap Disjoint Set Union (Union-Find) with path halving and union by size/rank for pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { DisjointSet } from "disjoint-sets";
```

## Core Types & Constants

```cp
class DisjointSet { parent: [256]u16, sizes: [256]u16, count: usize, components: usize } // methods: init(...) +8
```

## Minimal Usage

```cp
import { DisjointSet } from "disjoint-sets";

pub fn main() {
    // Refer to src/index.d.cp for DisjointSet fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"disjoint-sets"`; install via `cap add disjoint-sets`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add disjoint-sets
cpc check src/main.cp
```
