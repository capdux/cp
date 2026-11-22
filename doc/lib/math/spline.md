# `cap:spline` — Zero-Allocation Cubic Spline, PCHIP & Catmull-Rom Interpolation Library

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

Pure CP, zero-heap-allocation numerical spline interpolation library providing **Natural** and **Clamped** $C^2$ cubic splines (via an $O(n)$ Thomas tridiagonal solver), shape-preserving monotonic **Fritsch-Carlson PCHIP** Hermite interpolation, and **Catmull-Rom** local splines.

## Core Types

- **`CubicSegment`**: `{ x0, x1, a, b, c, d: f64 }` representing $S(x) = a + b(x - x_0) + c(x - x_0)^2 + d(x - x_0)^3$
  - `eval(x): f64`, `derivative(x): f64`, `secondDerivative(x): f64`, `integrate(): f64`
- **`CubicSpline`**: `{ segments: []CubicSegment }`
  - `eval(x): f64`, `derivative(x): f64`, `secondDerivative(x): f64` ($O(\log n)$ binary search), `integrate(): f64` (exact analytic integral)

## Core Functions

| Symbol | Description |
| :--- | :--- |
| `buildNaturalCubicSpline(xs, ys, segments, scratch): CubicSpline?` | Natural $C^2$ cubic spline ($S''(x_0) = S''(x_{n-1}) = 0$); requires `scratch.len >= 3 * xs.len`. |
| `buildClampedCubicSpline(xs, ys, dy0, dyn, segments, scratch): CubicSpline?` | Clamped $C^2$ cubic spline with prescribed endpoint slopes `dy0`, `dyn`. |
| `buildPchipSpline(xs, ys, segments): CubicSpline?` | Monotonicity-preserving Fritsch-Carlson PCHIP Hermite spline (zero overshoot, no scratch needed). |
| `catmullRom1d(p0, p1, p2, p3, t): f64` | Evaluates uniform Catmull-Rom segment from `p1` (`t=0`) to `p2` (`t=1`). |
| `catmullRom1dDeriv(p0, p1, p2, p3, t): f64` | Evaluates first derivative `dP/dt` of the Catmull-Rom segment. |

## Example

```cp
import { CubicSegment, buildPchipSpline } from "cap:math";

fn sampleStep(x: f64): f64 {
    const xs: [4]f64 = [0.0, 1.0, 2.0, 3.0];
    const ys: [4]f64 = [0.0, 0.0, 1.0, 1.0];
    let segs: [3]CubicSegment = undefined;
    let seg_slice: []CubicSegment = segs[0..];
    if (sp := buildPchipSpline(xs[0..], ys[0..], *seg_slice)) {
        return sp.eval(x);
    }
    return 0.0;
}
```
