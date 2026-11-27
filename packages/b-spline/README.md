[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# b-spline

Zero-heap general B-Spline / NURBS (Cox-de Boor), Cubic Hermite, Catmull-Rom, and uniform cubic B-Spline interpolation library in pure CP.

Aligned with npm [`b-spline`](https://github.com/thibauts/b-spline) and crates.io [`bspline`](https://github.com/Twinklebear/bspline) (MIT).

## Installation & Import

```bash
cap add b-spline
```

```cp
import {
    BSPLINE_MAX_DEGREE,
    BSPLINE_MAX_KNOTS,
    Pt2,
    Pt3,
    bspline1D,
    bspline2D,
    bspline3D,
    bspline2DUniform,
    generateClampedKnots,
    sampleBSpline2D,
    catmullRom2D,
    hermite2D,
} from "b-spline";
```

## Core API

### Constants & Types

| Symbol | Type / Fields | Description |
| --- | --- | --- |
| `BSPLINE_MAX_DEGREE` | `usize = 16` | Maximum spline degree supported on stack workspace. |
| `BSPLINE_MAX_KNOTS` | `usize = 256` | Maximum knot vector length supported on stack workspace. |
| `Pt2` | `{ x: f64, y: f64 }` | 2D control/sample point. |
| `Pt3` | `{ x: f64, y: f64, z: f64 }` | 3D control/sample point. |

### Spline & Curve Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hermite1D` / `hermite2D` | `(p0, m0, p1, m1, t: f64): f64 / Pt2` | Cubic Hermite interpolation given endpoints and tangents. |
| `catmullRom1D` / `catmullRom2D` | `(p0, p1, p2, p3, t: f64): f64 / Pt2` | 4-point Catmull-Rom segment interpolation. |
| `uniformCubicBSpline1D` / `2D` | `(p0, p1, p2, p3, t: f64): f64 / Pt2` | Uniform cubic B-Spline segment evaluation. |
| `evalCatmullRomChain` | `(pts: []const Pt2, u: f64): Pt2` | Evaluate a multi-point 2D Catmull-Rom chain at `u` in `[0, 1]`. |
| `generateClampedKnots` | `(num_points: usize, degree: usize, out_knots: []f64): usize` | Generate clamped open knot vector into `out_knots`; returns knot count. |
| `generateUniformKnots` | `(num_points: usize, degree: usize, out_knots: []f64): usize` | Generate uniform knot vector into `out_knots`; returns knot count. |
| `findKnotSpan` | `(n: usize, degree: usize, u: f64, knots: []const f64): usize` | Locate knot span index for parameter `u`. |
| `bspline1D` / `2D` / `3D` | `(t: f64, degree: usize, points, knots: []const f64, weights: []const f64): T?` | General rational/non-rational B-Spline evaluation (pass empty `knots`/`weights` for defaults). Returns `null` on invalid parameters. |
| `bspline1DUniform` / `2DUniform` / `3DUniform` | `(t: f64, degree: usize, points): T?` | Convenience uniform B-Spline evaluation without explicit knot/weight vectors. |
| `sampleBSpline2D` | `(degree: usize, points: []const Pt2, knots: []const f64, weights: []const f64, out: []Pt2): usize` | Uniformly sample a 2D B-Spline curve across `out.len` points; returns samples written. |

## Examples

### 1. Clamped 2D B-Spline Evaluation & Sampling

```cp
import { print } from "cap:io";
import { Pt2, generateClampedKnots, bspline2D, sampleBSpline2D } from "b-spline";

pub fn main() {
    let ctrl: [4]Pt2 = [
        Pt2 { x: 0.0, y: 0.0 },
        Pt2 { x: 1.0, y: 2.0 },
        Pt2 { x: 3.0, y: 2.0 },
        Pt2 { x: 4.0, y: 0.0 },
    ];
    let knots: [8]f64 = undefined;
    let klen = generateClampedKnots(4, 3, knots[0..8]);
    let empty_w: [0]f64 = [];

    let mid = bspline2D(0.5, 3, ctrl[0..4], knots[0..klen], empty_w[0..0]);
    if (mid != null) {
        print("mid=({}, {})\n", mid!.x, mid!.y);
    }

    let samples: [16]Pt2 = undefined;
    let n = sampleBSpline2D(3, ctrl[0..4], knots[0..klen], empty_w[0..0], samples[0..16]);
    print("sampled {} points\n", n);
}
```

### 2. Uniform 3D B-Spline Shortcut

```cp
import { print } from "cap:io";
import { Pt3, bspline3DUniform } from "b-spline";

pub fn main() {
    let pts: [4]Pt3 = [
        Pt3 { x: 0.0, y: 0.0, z: 0.0 },
        Pt3 { x: 1.0, y: 1.0, z: 0.5 },
        Pt3 { x: 2.0, y: 0.0, z: 1.0 },
        Pt3 { x: 3.0, y: 1.0, z: 1.5 },
    ];
    let p = bspline3DUniform(0.5, 2, pts[0..4]);
    if (p != null) {
        print("3D point: ({}, {}, {})\n", p!.x, p!.y, p!.z);
    }
}
```

### 3. Catmull-Rom Chain Interpolation

```cp
import { Pt2, evalCatmullRomChain } from "b-spline";

pub fn samplePath(pts: []const Pt2): Pt2 {
    return evalCatmullRomChain(pts, 0.25);
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: All De Boor recursion tables live on the stack (capped by `BSPLINE_MAX_DEGREE = 16` and `BSPLINE_MAX_KNOTS = 256`).
- **Typed 1D/2D/3D API**: Replaces dynamically sized JS number arrays with strongly typed `f64`, `Pt2`, and `Pt3` values for optimal SIMD/register codegen.
- **NURBS weights & extra splines**: Supports optional rational weights (`weights: []const f64`) plus Hermite and Catmull-Rom curve helpers in one zero-dependency package.
