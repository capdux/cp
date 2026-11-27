[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `mint`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:mint` ([https://github.com/kvark/mint](https://github.com/kvark/mint))

Zero-heap Math Interoperability Standard Types for CP graphics, physics, and scientific libraries — clean-room CP equivalent of crates.io `mint`.

## Features
- `Point2`, `Point3`, `Vector2`, `Vector3`, `Vector4` with `dot`, `cross`, `lenSq`, `toArray`
- `Quaternion` (`identity`, `conjugate`, `normSq`, `mul`) & `EulerAngles`
- `RowMatrix2`, `RowMatrix3`, `RowMatrix4`, `ColumnMatrix4` with `identity`, `det`, `transpose`, `mulVec3`, `mulVec4`, row↔column major conversion
- `point3ToVector3`, `vector3ToPoint3`
