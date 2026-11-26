# @cap/astar

Zero-heap 2D grid A* heuristic shortest-pathfinding engine in pure CP.

## Features

- Fixed-capacity `GridMap` up to `32 × 32` (`1024` cells), zero heap allocations
- Stack binary min-heap open set with O(log N) decrease-key
- 4-way Manhattan (`cost = 10`) and 8-way Octile diagonal (`cost = 14`, with corner-cutting guard)
- Full start-to-goal coordinate path reconstruction into caller buffer

## Usage

```cp
import { GridMap, GridPos } from "@cap/astar";

let map = GridMap.init(10, 10);
map.setBlocked(3, 2, true);

let path: [64]GridPos = undefined;
const res = map.findPath(
    GridPos { x: 0, y: 0 },
    GridPos { x: 9, y: 9 },
    true,
    path[0..64],
);
```
