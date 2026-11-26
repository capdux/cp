# @cap/ray-cast

Zero-heap-allocation 2D/3D geometric ray intersection library in pure CP, directly reusing `@cap/libm` (`sqrt`, `fabs`).

## Dependencies

- `@cap/libm` (`^1.0.0`)

## Features

- `V3`, `Ray3`, `HitResult { hit, t, u, v }`
- `rayAt(ray, t): V3`
- `rayPlane(ray, plane_pt, normal): HitResult`
- `raySphere(ray, center, radius): HitResult`
- `rayTriangle(ray, v0, v1, v2): HitResult` (Möller-Trumbore with barycentric `(u, v)`)
- `raySegment2D(ox, oy, dx, dy, ax, ay, bx, by): HitResult`

## License

MIT
