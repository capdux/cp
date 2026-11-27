# point-in-polygon — AI Agent Quick Ref

Zero-heap 2D polygon geometry (Shoelace area, centroid, perimeter, ray-casting containment, convexity) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Pt2,
    absF64,
    sqrtF64,
    signedArea,
    area,
    isCounterClockwise,
    perimeter,
    centroid,
} from "point-in-polygon";
```

## Core Types & Constants

```cp
class Pt2 { x: f64, y: f64 }
```

## API Signatures

```cp
fn absF64(x: f64): f64;
fn sqrtF64(x: f64): f64;
fn signedArea(pts: &[]Pt2): f64;
fn area(pts: &[]Pt2): f64;
fn isCounterClockwise(pts: &[]Pt2): bool;
fn perimeter(pts: &[]Pt2): f64;
fn centroid(pts: &[]Pt2): Pt2;
fn pointOnSegment(p: Pt2, a: Pt2, b: Pt2): bool;
fn containsPoint(pts: &[]Pt2, p: Pt2): bool;
fn isConvex(pts: &[]Pt2): bool;
```

## Minimal Usage

```cp
import { isCounterClockwise } from "point-in-polygon";

pub fn main() {
    let result = isCounterClockwise(/* pts: &[]Pt2 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"point-in-polygon"`; install via `cap add point-in-polygon`.
