# @cap/mat2

Zero-heap-allocation 2x2 matrix (`Mat2`) and 2D linear transformation library in pure CP with zero external dependencies (built-in range-reduced Taylor series `sinF64` / `cosF64`).

## Features

- **Types**: `Mat2 { m00, m01, m10, m11 }` (row-major), `V2 { x, y }`
- **Constructors**: `identity()`, `zero()`, `fromScale(sx, sy)`, `fromRotation(radians)`, `fromShear(shx, shy)`
- **Operations**: `add`, `sub`, `scale`, `mul`, `mulVec`, `transpose`, `trace`, `determinant`, `isInvertible`, `inverse`, `frobeniusNormSq`, `approxEq`
- Both method syntax and top-level functional helpers are exported.
- Singular matrices safely return `Mat2.zero()` from `inverse()` without `NaN` or division by zero.

## License

MIT
