# astar — AI Agent Quick Ref

Zero-heap 2D grid A* shortest pathfinding engine with 4-way Manhattan & 8-way Octile diagonal search and path reconstruction. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    GridPos,
    AStarResult,
    GridMap,
    absDiffUsize,
    heuristic,
} from "astar";
```

## Core Types & Constants

```cp
class GridPos { x: u16, y: u16 }
class AStarResult { found: bool, cost: i64, path_len: usize }
class GridMap { width: usize, height: usize, blocked: [1024]bool } // methods: init(...) +3
```

## API Signatures

```cp
fn absDiffUsize(a: usize, b: usize): i64;
fn heuristic(x1: usize, y1: usize, x2: usize, y2: usize, allow_diagonal: bool): i64;
```

## Minimal Usage

```cp
import { absDiffUsize } from "astar";

pub fn main() {
    let result = absDiffUsize(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"astar"`; install via `cap add astar`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
