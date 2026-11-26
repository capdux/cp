# @cap/bezier-curve

Zero-heap-allocation quadratic & cubic Bezier curve evaluation, first derivatives, de Casteljau splitting, chord-sum arc length approximation, and CSS `cubic-bezier(x1, y1, x2, y2)` easing solver in pure CP.

## Features

- `Pt2 { x, y }`: 2D point struct
- `QuadBezier { p0, p1, p2 }`: `eval(t)`, `deriv(t)`, `arcLength(steps)`
- `CubicBezier { p0, p1, p2, p3 }`: `eval(t)`, `deriv(t)`, `arcLength(steps)`
- `splitCubic(c, t) -> CubicSplit { left, right }`: exact de Casteljau subdivision
- `cubicBezierEasing(x1, y1, x2, y2, x)`: Newton-Raphson + bisection fallback solver matching CSS `cubic-bezier()`
- Zero external dependencies (built-in Newton-Raphson `sqrtF64`)

## License

MIT
