# kiddo — AI Agent Quick Ref

Zero-allocation static 2D KD-Tree spatial index with exact nearest-neighbor and radius range queries in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    KdPoint2D,
    NearestResult,
    KdNode,
    BuildTask,
    SearchFrame,
    KdTree2D,
    coordAt,
    sortRangeByAxis,
} from "kiddo";
```

## Core Types & Constants

```cp
class KdPoint2D { x: f64, y: f64, id: u32 }
class NearestResult { id: u32, dist_sq: f64, found: bool }
class KdNode { x: f64, y: f64, id: u32, axis: u8, /* +2 fields */ }
class BuildTask { lo: usize, hi: usize, depth: usize, slot: usize }
class SearchFrame { idx: i32, plane_dist_sq: f64 }
class KdTree2D { nodes: [256]KdNode, node_count: usize } // methods: build(...) +4
```

## API Signatures

```cp
fn coordAt(p: KdPoint2D, axis: u8): f64;
fn sortRangeByAxis(pts: []KdPoint2D, lo: usize, hi: usize, axis: u8);
```

## Minimal Usage

```cp
import { coordAt } from "kiddo";

pub fn main() {
    let result = coordAt(/* p: KdPoint2D */, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"kiddo"`; install via `cap add kiddo`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
