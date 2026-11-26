# `@cap/nalgebra`

Zero-heap 2D/3D/4D linear algebra library with vector operations, matrix multiplication, determinant, inverse, and 3D affine transforms — clean-room CP equivalent of crates.io `nalgebra`.

Reuses `@cap/mint` as an independent package.

## Features
- `vec2Add`/`Sub`/`Scale`/`Norm`, `vec3Add`/`Sub`/`Scale`/`Norm`/`Normalize`/`Lerp`/`Distance`
- `mat2Mul`, `mat2Inverse`, `mat3Det`, `mat3Mul`, `mat3Inverse`
- `mat4Mul`, `mat4Translation`, `mat4Scaling`, `mat4TransformPoint`
