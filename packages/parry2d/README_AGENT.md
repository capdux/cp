# parry2d@1.0.0 (Agent Quickref)

> Zero-allocation 2D axis-aligned bounding box (AABB) collision detection, union, intersection, and ray slab intersection for CP | License: MIT
> Import: `import { ... } from "parry2d";` | Full types: `src/index.d.cp`

## Functions
- `export fn minF64(a: f64, b: f64): f64;`
- `export fn maxF64(a: f64, b: f64): f64;`
- `export fn new(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Aabb2;`
- `export fn fromCenterSize(cx: f64, cy: f64, w: f64, h: f64): Aabb2;`
- `export fn fromPoints(pts: &[]Pt2): Aabb2;`
