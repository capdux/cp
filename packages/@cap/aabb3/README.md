# @cap/aabb3

Zero-allocation 3D Axis-Aligned Bounding Box (AABB) collision detection, union/intersection, surface area/volume, and Kay-Kajiya Slab raycast engine for CP.

## Features

- **Zero heap allocation**: pure CP value types `Aabb3` and `Pt3`.
- **Bounding box geometry**: `new`, `fromCenterSize`, `isValid`, `volume`, `surfaceArea`, `center`, `expandBy`, `distanceSqToPoint`.
- **Spatial queries**: `containsPoint`, `intersects`, `intersection`, `unionBox`, and parallel-axis-safe 3D `rayIntersects`.

## Usage

```cp
import { Aabb3 } from "@cap/aabb3";

const box = Aabb3.fromCenterSize(0.0 as f64, 0.0 as f64, 0.0 as f64, 2.0 as f64, 2.0 as f64, 2.0 as f64);
const hit = box.rayIntersects(-5.0 as f64, 0.0 as f64, 0.0 as f64, 1.0 as f64, 0.0 as f64, 0.0 as f64); // true
```
