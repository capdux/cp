[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# glam

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:glam` ([https://github.com/bitshifter/glam-rs](https://github.com/bitshifter/glam-rs))

Zero-heap 3D math library (`Vec3`, unit `Quat`, column-major `Mat4`) in pure CP with zero external dependencies.

## Features

- `Vec3`: `init`, `add`, `sub`, `scale`, `dot`, `cross`, `lengthSq`, `length`, `normalize` (NaN-safe), `lerp`, `distance`
- `Quat`: `identity`, `fromAxisAngle`, `normalize`, `conjugate`, `mul`, `rotateVec3`
- `Mat4` (column-major): `identity`, `translation`, `scaling`, `rotationZ`, `mul`, `transformPoint`, `determinant`

## Usage

```cp
import { Vec3, Quat, Mat4 } from "glam";

const q = Quat.fromAxisAngle(Vec3.init(0.0, 0.0, 1.0), 1.5707963267948966);
const rotated = q.rotateVec3(Vec3.init(1.0, 0.0, 0.0)); // (0, 1, 0)
```
