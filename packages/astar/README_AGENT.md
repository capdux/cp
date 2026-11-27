# astar@1.0.0 (Agent Quickref)

> Zero-heap 2D grid A* shortest pathfinding engine with 4-way Manhattan & 8-way Octile diagonal search and path reconstruction. | License: MIT
> Import: `import { ... } from "astar";` | Full types: `src/index.d.cp`

## Functions
- `export fn absDiffUsize(a: usize, b: usize): i64;`
- `export fn heuristic(x1: usize, y1: usize, x2: usize, y2: usize, allow_diagonal: bool): i64;`
