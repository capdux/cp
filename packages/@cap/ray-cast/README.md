<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`parry3d`](https://github.com/dimforge/parry)（`Apache-2.0`）（原库名 `parry3d`，CP 包名 `@cap/ray-cast`）。

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
