# topological-sort — AI Agent Quick Ref

Zero-allocation deterministic Kahn topological sort, cycle detection, and parallel DAG level stratification (Rust topological-sort / petgraph / npm toposort parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { TopoResult, TopoGraph, clampNodes } from "topological-sort";
```

## Core Types & Constants

```cp
class TopoResult { ok: bool, count: usize, order: [MAX_NODES]u16, levels: [MAX_NODES]u16 } // methods: empty(...)
class TopoGraph { node_count: usize, edge_count: usize, head: [MAX_NODES]i16, edge_to: [MAX_EDGES]u16, /* +3 fields */ } // methods: init(...) +6
```

## API Signatures

```cp
fn clampNodes(n: usize): usize;
```

## Minimal Usage

```cp
import { clampNodes } from "topological-sort";

pub fn main() {
    let result = clampNodes(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"topological-sort"`; install via `cap add topological-sort`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add topological-sort
cpc check src/main.cp
```
