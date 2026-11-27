# simplify-js@1.0.0 (Agent Quickref)

> Zero-heap Ramer-Douglas-Peucker 2D polyline simplification for GPS tracks and vector graphics in pure CP | License: MIT
> Import: `import { ... } from "simplify-js";` | Full types: `src/index.d.cp`

## Functions
- `export fn pointToSegmentDistSq(p: Pt2, a: Pt2, b: Pt2): f64;`
- `export fn simplifyPolyline(pts: []Pt2, epsilon: f64, keep_mask: []u8, out: []Pt2): usize;`
