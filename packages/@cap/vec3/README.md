# @cap/vec3

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:gl-matrix` ([https://github.com/toji/gl-matrix](https://github.com/toji/gl-matrix))（原库名 `gl-matrix` → CP 包名 `@cap/vec3`）

High-performance, zero-heap-allocation 3D vector (`Vec3`) and spatial geometry library in pure CP with zero external dependencies (uses built-in Newton-Raphson `sqrtF64`).

## Features

- **Constructors**: `Vec3.new(x, y, z)`, `Vec3.zero()`, `Vec3.splat(v)`, `Vec3.unitX()`, `Vec3.unitY()`, `Vec3.unitZ()`
- **Arithmetic**: `add`, `sub`, `mul`, `scale`, `neg`
- **Geometry**: `dot`, `cross`, `lengthSq`, `length`, `distance`, `normalize`, `lerp`, `reflect`, `project`, `minVec`, `maxVec`, `clampLength`, `approxEq`
- Both method syntax (`a.cross(b)`) and top-level functional helpers (`cross(a, b)`) are exported.
- Safe on zero vectors (`normalize`, `project`, `clampLength` never produce `NaN`).

## Usage

```cp
import { Vec3 } from "@cap/vec3";

let x = Vec3.unitX();
let y = Vec3.unitY();
let z = x.cross(y); // Vec3 { x: 0.0, y: 0.0, z: 1.0 }
let n = Vec3.new(3.0, 4.0, 0.0).normalize(); // (0.6, 0.8, 0.0)
```

## License

MIT
