# kdbush — AI Agent Quick Ref

Pure CP zero-allocation static 2D K-D Tree with balanced median-split build, hyperplane-pruned nearest neighbor, rectangle and radius queries (kdbush / kiddo parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    KdPt,
    KdNode,
    NearestResult,
    KdBuildFrame,
    KdSearchFrame,
    kdCoord,
    kdSwap,
    kdInsertionSort,
} from "kdbush";
```

## Core Types & Constants

```cp
class KdPt { x: f64, y: f64, id: u32 }
class KdNode { pt: KdPt, left: i32, right: i32, axis: u8 }
class NearestResult { found: bool, id: u32, dist_sq: f64 }
class KdBuildFrame { lo: usize, hi: usize, axis: u8, parent: i32, /* +1 fields */ }
class KdSearchFrame { idx: i32, min_dist_sq: f64 }
```

## API Signatures

```cp
fn kdCoord(pt: KdPt, axis: u8): f64;
fn kdSwap(pts: []KdPt, i: usize, j: usize);
fn kdInsertionSort(pts: []KdPt, lo: usize, hi: usize, axis: u8);
fn kdSelectMedian(pts: []KdPt, lo_in: usize, hi_in: usize, target: usize, axis: u8);
fn buildKdTree(pts: []KdPt, nodes: []KdNode): i32;
fn nearestNeighbor(nodes: []KdNode, root: i32, qx: f64, qy: f64): NearestResult;
fn rangeQueryRect(nodes: []KdNode, root: i32, min_x: f64, min_y: f64, max_x: f64, max_y: f64, out_ids: []u32): usize;
fn radiusQuery(nodes: []KdNode, root: i32, cx: f64, cy: f64, radius: f64, out_ids: []u32): usize;
```

## Minimal Usage

```cp
import { kdCoord } from "kdbush";

pub fn main() {
    let result = kdCoord(/* pt: KdPt */, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"kdbush"`; install via `cap add kdbush`.
