# kiddo@1.0.0 (Agent Quickref)

> Zero-allocation static 2D KD-Tree spatial index with exact nearest-neighbor and radius range queries in pure CP | License: MIT
> Import: `import { ... } from "kiddo";` | Full types: `src/index.d.cp`

## Functions
- `export fn coordAt(p: KdPoint2D, axis: u8): f64;`
- `export fn sortRangeByAxis(pts: []KdPoint2D, lo: usize, hi: usize, axis: u8);`
