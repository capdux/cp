# petgraph — AI Agent Quick Ref

Zero-allocation directed/undirected graph and classic graph algorithms (BFS, iterative DFS, Kahn topoSort, cycle detection, Dijkstra) (Rust petgraph v0.6 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { DiGraph, pgcSatAdd } from "petgraph";
```

## Core Types & Constants

```cp
class DiGraph { node_count: usize, edge_count: usize, edge_from: [256]usize, edge_to: [256]usize, /* +6 fields */ } // methods: new(...) +14
```

## API Signatures

```cp
fn pgcSatAdd(a: u64, b: u64): u64;
```

## Minimal Usage

```cp
import { pgcSatAdd } from "petgraph";

pub fn main() {
    let result = pgcSatAdd(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"petgraph"`; install via `cap add petgraph`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add petgraph
cpc check src/main.cp
```
