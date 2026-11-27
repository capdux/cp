# pathfinding — AI Agent Quick Ref

Zero-allocation weighted shortest-path engine with Dijkstra, Bellman-Ford negative-cycle detection, and path reconstruction (Rust pathfinding / petgraph parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { ShortestPaths, WeightedGraph, emptyShortestPaths } from "pathfinding";
```

## Core Types & Constants

```cp
class ShortestPaths { ok: bool, node_count: usize, source: usize, dist: [MAX_NODES]i64, /* +1 fields */ } // methods: isReachable(...) +2
class WeightedGraph { node_count: usize, edge_count: usize, head: [MAX_NODES]i16, edge_src: [MAX_EDGES]u16, /* +3 fields */ } // methods: init(...) +7
```

## API Signatures

```cp
fn emptyShortestPaths(n_nodes: usize, src: usize, ok_val: bool): ShortestPaths;
```

## Minimal Usage

```cp
import { emptyShortestPaths } from "pathfinding";

pub fn main() {
    let result = emptyShortestPaths(0, 0, false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pathfinding"`; install via `cap add pathfinding`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add pathfinding
cpc check src/main.cp
```
