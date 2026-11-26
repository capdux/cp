# `@cap/quat`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:quaternion` ([https://github.com/rawify/Quaternion.js](https://github.com/rawify/Quaternion.js))（原库名 `quaternion` → CP 包名 `@cap/quat`）

Zero-heap 3D unit quaternion rotation, SLERP / NLERP interpolation, and Euler angle conversion in pure CP with zero external math dependencies.

## Features

- **100% Zero Heap Allocation & Zero Dependencies**: Built-in Newton-Raphson `sqrtF64`, Taylor-series `sinF64` / `cosF64` / `atanF64` / `acosF64`.
- **Full 3D Rotation API**: `new`, `identity`, `fromAxisAngle`, `fromEulerXYZ`, `mul`, `conjugate`, `inverse`, `dot`, `length`, `normalize`, `nlerp`, `slerp`, `rotatePoint`, `approxEq`.
- **Safe Boundary Handling**: Zero-length axes and quaternions safely fall back to `identity()`.

## Quick Start

```cp
import { HALF_PI, Point3, fromAxisAngle, rotatePoint, slerp, identity } from "@cap/quat";

const q90 = fromAxisAngle(0.0, 0.0, 1.0, HALF_PI);
const p = rotatePoint(q90, Point3 { x: 1.0, y: 0.0, z: 0.0 }); // (0, 1, 0)
const mid = slerp(identity(), q90, 0.5); // 45-degree Z rotation
```

## License

MIT
