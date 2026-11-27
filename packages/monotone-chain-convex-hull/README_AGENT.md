# monotone-chain-convex-hull — AI Agent Quick Ref

Zero-heap 2D Andrew's monotone chain convex hull, area, and perimeter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Pt2,
    absF64,
    sqrtF64,
    ptLess,
    ptEqual,
    crossOAB,
    sortPointsXY,
    convexHull,
} from "monotone-chain-convex-hull";
```

## Core Types & Constants

```cp
class Pt2 { x: f64, y: f64 }
```

## API Signatures

```cp
fn absF64(x: f64): f64;
fn sqrtF64(x: f64): f64;
fn ptLess(a: Pt2, b: Pt2): bool;
fn ptEqual(a: Pt2, b: Pt2): bool;
fn crossOAB(o: Pt2, a: Pt2, b: Pt2): f64;
fn sortPointsXY(pts: []Pt2);
fn convexHull(work_pts: []Pt2, out_hull: []Pt2): usize;
fn hullArea(hull: &[]Pt2): f64;
fn hullPerimeter(hull: &[]Pt2): f64;
```

## Minimal Usage

```cp
import { absF64 } from "monotone-chain-convex-hull";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"monotone-chain-convex-hull"`; install via `cap add monotone-chain-convex-hull`.
