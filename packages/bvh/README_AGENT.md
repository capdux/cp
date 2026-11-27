# bvh — AI Agent Quick Ref

Zero-allocation static 3D AABB Bounding Volume Hierarchy (BVH) with overlap queries and slab raycasting in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Aabb,
    BvhItem,
    RayHit,
    BvhNode,
    BuildTask,
    RayFrame,
    Bvh,
    minF64,
} from "bvh";
```

## Core Types & Constants

```cp
class Aabb { min_x: f64, min_y: f64, min_z: f64, max_x: f64, /* +2 fields */ } // methods: merge(...) +4
class BvhItem { bounds: Aabb, id: u32 }
class RayHit { id: u32, t: f64, hit: bool }
class BvhNode { bounds: Aabb, left: i32, right: i32, item_id: u32, /* +1 fields */ }
class BuildTask { lo: usize, hi: usize, slot: usize }
class RayFrame { idx: i32, t_enter: f64 }
class Bvh { nodes: [256]BvhNode, node_count: usize, item_count: usize } // methods: build(...) +4
```

## API Signatures

```cp
fn minF64(a: f64, b: f64): f64;
fn maxF64(a: f64, b: f64): f64;
fn centroidOnAxis(b: Aabb, axis: u8): f64;
fn sortItemsByAxis(items: []BvhItem, lo: usize, hi: usize, axis: u8);
```

## Minimal Usage

```cp
import { minF64 } from "bvh";

pub fn main() {
    let result = minF64(0.0, 0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bvh"`; install via `cap add bvh`.
