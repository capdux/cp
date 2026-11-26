# `@cap/bspline`

Zero-heap cubic Hermite, Catmull-Rom, and uniform cubic B-Spline curve interpolation in pure CP.

## Features

- **100% Zero Heap Allocation**: Pure value-type math over `f64` and `Pt2`.
- **Cubic Hermite**: `hermite1D` / `hermite2D` with exact endpoint values and tangents.
- **Catmull-Rom**: `catmullRom1D` / `catmullRom2D` + multi-segment `evalCatmullRomChain(pts, u)`.
- **Uniform Cubic B-Spline**: `uniformCubicBSpline1D` / `uniformCubicBSpline2D` with convex hull & collinearity preservation.

## Quick Start

```cp
import { Pt2, evalCatmullRomChain, uniformCubicBSpline2D } from "@cap/bspline";

const path: [4]Pt2 = [
    Pt2 { x: 0.0, y: 0.0 },
    Pt2 { x: 1.0, y: 2.0 },
    Pt2 { x: 3.0, y: 2.0 },
    Pt2 { x: 4.0, y: 0.0 },
];
const mid = evalCatmullRomChain(&path, 0.5);
```

## License

MIT
