# simplify-js — AI Agent Quick Ref

Zero-heap 2D/3D Ramer-Douglas-Peucker & radial-distance polyline simplification in pure CP (npm `simplify-js` parity).

## Import

```cp
import {
    SIMPLIFY_MAX_STACK_PTS,
    Pt2, Point2D, Point3D,
    getSqDist, pointToSegmentDistSq, getSqSegDist,
    getSqDist3D, getSqSegDist3D,
    simplify, simplifyRadialDist, simplifyDouglasPeucker, simplifyPolyline,
    simplify3D, simplifyRadialDist3D, simplifyDouglasPeucker3D,
} from "simplify-js";
```

## API Signatures

```cp
const SIMPLIFY_MAX_STACK_PTS: usize; // 1024
class Pt2 { x: f64, y: f64 }
type Point2D = Pt2;
class Point3D { x: f64, y: f64, z: f64 }

fn getSqDist(p1: Pt2, p2: Pt2): f64;
fn pointToSegmentDistSq(p: Pt2, a: Pt2, b: Pt2): f64;
fn getSqSegDist(p: Pt2, p1: Pt2, p2: Pt2): f64;
fn getSqDist3D(p1: Point3D, p2: Point3D): f64;
fn getSqSegDist3D(p: Point3D, a: Point3D, b: Point3D): f64;
fn simplifyRadialDist(points: []const Pt2, sqTolerance: f64, out: []Pt2): usize;
fn simplifyDouglasPeucker(points: []const Pt2, sqTolerance: f64, out: []Pt2): usize;
fn simplify(points: []const Pt2, tolerance: f64, highQuality: bool, out: []Pt2): usize;
fn simplifyPolyline(pts: []Pt2, epsilon: f64, keep_mask: []u8, out: []Pt2): usize;
fn simplifyRadialDist3D(points: []const Point3D, sqTolerance: f64, out: []Point3D): usize;
fn simplifyDouglasPeucker3D(points: []const Point3D, sqTolerance: f64, out: []Point3D): usize;
fn simplify3D(points: []const Point3D, tolerance: f64, highQuality: bool, out: []Point3D): usize;
```

## Minimal Example

```cp
import { Pt2, simplify } from "simplify-js";

let pts: [3]Pt2 = [Pt2 { x: 0.0, y: 0.0 }, Pt2 { x: 1.0, y: 0.1 }, Pt2 { x: 2.0, y: 0.0 }];
let out: [3]Pt2 = undefined;
let n = simplify(pts[0..3], 0.5, true, out[0..3]); // n == 2
```

## Pitfalls & Borrowing Rules

- **`tolerance` vs `sqTolerance`**: `simplify` / `simplify3D` / `simplifyPolyline` take linear tolerance; `simplifyRadialDist*` and `simplifyDouglasPeucker*` take **squared** tolerance (`tol * tol`).
- **Output capacity**: Ensure `out.len >= points.len` to avoid truncation; return value is the exact number of points written.
- **Large polylines**: Built-in `simplify*` stack buffers cap at `SIMPLIFY_MAX_STACK_PTS` (1024); for larger 2D tracks use `simplifyPolyline` with caller-supplied `keep_mask: []u8` (`keep_mask.len >= pts.len`).
