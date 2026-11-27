[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# simplify-js

Zero-heap Ramer-Douglas-Peucker (RDP) and Radial Distance 2D/3D polyline simplification library for GPS tracks, GIS geometry, and vector graphics in pure CP.

Aligned with npm [`simplify-js`](https://github.com/mourner/simplify-js) (MIT).

## Installation & Import

```bash
cap add simplify-js
```

```cp
import {
    SIMPLIFY_MAX_STACK_PTS,
    Pt2,
    Point2D,
    Point3D,
    simplify,
    simplify3D,
    simplifyRadialDist,
    simplifyDouglasPeucker,
    simplifyPolyline,
} from "simplify-js";
```

## Core API

### Constants & Types

| Symbol | Type / Fields | Description |
| --- | --- | --- |
| `SIMPLIFY_MAX_STACK_PTS` | `usize = 1024` | Maximum stack point capacity for built-in RDP working buffers. |
| `Pt2` / `Point2D` | `{ x: f64, y: f64 }` | 2D point struct (`Point2D` is a type alias for `Pt2`). |
| `Point3D` | `{ x: f64, y: f64, z: f64 }` | 3D point struct for elevation/3D trajectory simplification. |

### Distance & Simplification Functions

| Function | Signature | Description |
| --- | --- | --- |
| `getSqDist` | `(p1: Pt2, p2: Pt2): f64` | Squared Euclidean distance between two 2D points. |
| `pointToSegmentDistSq` | `(p: Pt2, a: Pt2, b: Pt2): f64` | Squared perpendicular distance from 2D point `p` to segment `ab`. |
| `getSqSegDist` | `(p: Pt2, p1: Pt2, p2: Pt2): f64` | Alias for `pointToSegmentDistSq`. |
| `getSqDist3D` | `(p1: Point3D, p2: Point3D): f64` | Squared Euclidean distance between two 3D points. |
| `getSqSegDist3D` | `(p: Point3D, a: Point3D, b: Point3D): f64` | Squared distance from 3D point `p` to segment `ab`. |
| `simplifyRadialDist` | `(points: []const Pt2, sqTolerance: f64, out: []Pt2): usize` | Fast radial-distance 2D pre-filter; writes kept points into `out` and returns count. |
| `simplifyDouglasPeucker` | `(points: []const Pt2, sqTolerance: f64, out: []Pt2): usize` | 2D RDP simplification taking squared tolerance; returns output point count. |
| `simplify` | `(points: []const Pt2, tolerance: f64, highQuality: bool, out: []Pt2): usize` | Main 2D entry point; skips radial pre-pass when `highQuality` is `true`. |
| `simplifyPolyline` | `(pts: []Pt2, epsilon: f64, keep_mask: []u8, out: []Pt2): usize` | Caller-supplied mask RDP for arbitrarily long 2D polylines beyond stack cap. |
| `simplifyRadialDist3D` | `(points: []const Point3D, sqTolerance: f64, out: []Point3D): usize` | Fast radial-distance 3D simplification. |
| `simplifyDouglasPeucker3D` | `(points: []const Point3D, sqTolerance: f64, out: []Point3D): usize` | 3D RDP simplification using squared tolerance. |
| `simplify3D` | `(points: []const Point3D, tolerance: f64, highQuality: bool, out: []Point3D): usize` | Main 3D entry point; returns number of simplified points written to `out`. |

## Examples

### 1. Standard 2D Polyline Simplification

```cp
import { print } from "cap:io";
import { Pt2, simplify } from "simplify-js";

pub fn main() {
    let pts: [5]Pt2 = [
        Pt2 { x: 0.0, y: 0.0 },
        Pt2 { x: 1.0, y: 0.1 },
        Pt2 { x: 2.0, y: -0.1 },
        Pt2 { x: 3.0, y: 5.0 },
        Pt2 { x: 4.0, y: 6.0 },
    ];
    let out: [5]Pt2 = undefined;
    let n = simplify(pts[0..5], 1.0, true, out[0..5]);
    print("kept {} points\n", n);
}
```

### 2. 3D GPS Track Simplification

```cp
import { print } from "cap:io";
import { Point3D, simplify3D } from "simplify-js";

pub fn main() {
    let track: [4]Point3D = [
        Point3D { x: 0.0, y: 0.0, z: 10.0 },
        Point3D { x: 1.0, y: 0.0, z: 10.1 },
        Point3D { x: 2.0, y: 0.0, z: 10.0 },
        Point3D { x: 3.0, y: 2.0, z: 15.0 },
    ];
    let out: [4]Point3D = undefined;
    let count = simplify3D(track[0..4], 0.5, false, out[0..4]);
    print("3D kept {} points\n", count);
}
```

### 3. Arbitrary-Length Polyline with Caller Mask

```cp
import { Pt2, simplifyPolyline } from "simplify-js";

pub fn compress(pts: []Pt2, mask: []u8, out: []Pt2): usize {
    return simplifyPolyline(pts, 0.25, mask, out);
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Caller-owned output slices**: Instead of allocating a JS array, all functions write directly into caller-provided `out: []Pt2` / `[]Point3D` buffers and return the written `usize` count.
- **Native 3D support**: Includes `Point3D`, `simplify3D`, `simplifyRadialDist3D`, and `simplifyDouglasPeucker3D` out of the box.
- **Non-recursive iterative RDP**: Uses bounded stack frames (`SIMPLIFY_MAX_STACK_PTS = 1024`) or caller-provided `keep_mask` (`simplifyPolyline`) to prevent stack overflow on pathological inputs.
