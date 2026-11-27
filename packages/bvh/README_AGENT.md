# bvh@1.0.0 (Agent Quickref)

> Zero-allocation static 3D AABB Bounding Volume Hierarchy (BVH) with overlap queries and slab raycasting in pure CP | License: MIT
> Import: `import { ... } from "bvh";` | Full types: `src/index.d.cp`

## Functions
- `export fn minF64(a: f64, b: f64): f64;`
- `export fn maxF64(a: f64, b: f64): f64;`
- `export fn centroidOnAxis(b: Aabb, axis: u8): f64;`
- `export fn sortItemsByAxis(items: []BvhItem, lo: usize, hi: usize, axis: u8);`
