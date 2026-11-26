# @cap/mat4

Zero-heap-allocation 4x4 row-major 3D graphics transformation matrix library in pure CP with zero external dependencies (built-in Taylor-series `sinF64` / `cosF64` / `tanF64` and Newton-Raphson `sqrtF64`).

## Features

- **Types**: `Mat4 { m: [16]f64 }` (row-major), `Vec3D { x, y, z }`
- **Constructors**: `identity`, `zero`, `fromTranslation`, `fromScale`, `fromRotationX`, `fromRotationY`, `fromRotationZ`, `orthographic`, `perspective`, `lookAt`
- **Operations**: `mul`, `transpose`, `transformPoint` (homogeneous perspective divide), `transformDir` (`w = 0`), `approxEq`

## License

MIT
