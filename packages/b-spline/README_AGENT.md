# b-spline — AI Agent Quick Ref

Zero-heap general B-Spline / NURBS (De Boor), Cubic Hermite, and Catmull-Rom interpolation in pure CP (npm `b-spline` / crates.io `bspline` parity).

## Import

```cp
import {
    BSPLINE_MAX_DEGREE, BSPLINE_MAX_KNOTS,
    Pt2, Pt3,
    hermite1D, hermite2D, catmullRom1D, catmullRom2D,
    uniformCubicBSpline1D, uniformCubicBSpline2D, evalCatmullRomChain,
    generateClampedKnots, generateUniformKnots, findKnotSpan,
    bspline1D, bspline2D, bspline3D,
    bspline1DUniform, bspline2DUniform, bspline3DUniform,
    sampleBSpline2D,
} from "b-spline";
```

## API Signatures

```cp
const BSPLINE_MAX_DEGREE: usize; // 16
const BSPLINE_MAX_KNOTS: usize;  // 256
class Pt2 { x: f64, y: f64 }
class Pt3 { x: f64, y: f64, z: f64 }

fn hermite1D(p0: f64, m0: f64, p1: f64, m1: f64, t: f64): f64;
fn hermite2D(p0: Pt2, m0: Pt2, p1: Pt2, m1: Pt2, t: f64): Pt2;
fn catmullRom1D(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64;
fn catmullRom2D(p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2, t: f64): Pt2;
fn uniformCubicBSpline1D(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64;
fn uniformCubicBSpline2D(p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2, t: f64): Pt2;
fn evalCatmullRomChain(pts: []const Pt2, u: f64): Pt2;
fn generateClampedKnots(num_points: usize, degree: usize, out_knots: []f64): usize;
fn generateUniformKnots(num_points: usize, degree: usize, out_knots: []f64): usize;
fn findKnotSpan(n: usize, degree: usize, u: f64, knots: []const f64): usize;
fn bspline1D(t: f64, degree: usize, points: []const f64, knots: []const f64, weights: []const f64): f64?;
fn bspline2D(t: f64, degree: usize, points: []const Pt2, knots: []const f64, weights: []const f64): Pt2?;
fn bspline3D(t: f64, degree: usize, points: []const Pt3, knots: []const f64, weights: []const f64): Pt3?;
fn bspline1DUniform(t: f64, degree: usize, points: []const f64): f64?;
fn bspline2DUniform(t: f64, degree: usize, points: []const Pt2): Pt2?;
fn bspline3DUniform(t: f64, degree: usize, points: []const Pt3): Pt3?;
fn sampleBSpline2D(degree: usize, points: []const Pt2, knots: []const f64, weights: []const f64, out: []Pt2): usize;
```

## Minimal Example

```cp
import { Pt2, bspline2DUniform } from "b-spline";

let ctrl: [4]Pt2 = [Pt2{x:0.0,y:0.0}, Pt2{x:1.0,y:2.0}, Pt2{x:3.0,y:2.0}, Pt2{x:4.0,y:0.0}];
let p = bspline2DUniform(0.5, 3, ctrl[0..4]);
if (p != null) { let x = p!.x; }
```

## Pitfalls & Borrowing Rules

- **Knot count formula**: `knots.len` must be `num_points + degree + 1` (or empty `[]` to auto-generate uniform knots); `out_knots` for `generate*Knots` needs at least `num_points + degree + 1` slots.
- **Optional returns**: `bspline*` returns `null` when `degree == 0`, `degree > BSPLINE_MAX_DEGREE` (16), `points.len <= degree`, or knot/weight counts mismatch.
- **Weights**: Pass an empty slice (`[]const f64`) for standard non-rational B-Splines, or a slice matching `points.len` for NURBS.
- **Immutability**: All input point/knot/weight slices are read-only borrows (`[]const T`).
