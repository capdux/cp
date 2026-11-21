# `cap:quat` — Zero-Allocation 3D Unit Quaternion, Euler ZYX, Mat3 & SLERP Library

Pure CP, zero-heap-allocation 3D rotation and attitude library providing Hamilton quaternions, 3D vectors, row-major 3×3 rotation matrices (with Shepperd's stable extraction), intrinsic ZYX Euler angles (with gimbal-lock handling), `fromToRotation` (including 180° antiparallel singularity), and shortest-arc SLERP/NLERP interpolation.

## Core Types

- **`Vec3`**: `{ x, y, z: f64 }` with `add`, `sub`, `scale`, `dot`, `cross`, `len`, `normalize`
- **`EulerAngles`**: `{ roll, pitch, yaw: f64 }` (intrinsic ZYX, radians)
- **`Mat3`**: `{ m: [9]f64 }` (row-major) with `mulVec3`, `transpose`, `det`
- **`Quat`**: `{ w, x, y, z: f64 }`
  - Constructors: `identity()`, `fromAxisAngle(axis, angle_rad): Quat?`, `fromEulerZYX(roll, pitch, yaw)`, `fromMat3(mat)` (Shepperd), `fromToRotation(from, to): Quat?`
  - Algebra: `norm`, `normSq`, `normalize`, `conjugate`, `inverse`, `negate`, `dot`, `mul`
  - Rotation & Conversion: `rotateVec3(v)`, `toMat3()`, `toEulerZYX()`, `angleTo(other)`
  - Interpolation: `nlerp(a, b, t)`, `slerp(a, b, t)` (shortest-arc, auto NLERP fallback near `theta -> 0`)

## Example

```cp
import std from "std";
import { Vec3, Quat } from "cap:quat";

fn rotateMidpoint(v: Vec3): Vec3 {
    const q0 = Quat.identity();
    if (q1 := Quat.fromAxisAngle(Vec3 { x: 0.0, y: 0.0, z: 1.0 }, std.math.pi * 0.5)) {
        const mid = Quat.slerp(q0, q1, 0.5);
        return mid.rotateVec3(v);
    }
    return v;
}
```
