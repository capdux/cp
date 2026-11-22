# `cap:geohash_3d` — 纯 CP 3D Morton Z-Order 空间编码与球面测距引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:geo`，导入请使用 `import { ... } from "cap:geo"`。

## 概述

`cap:geohash_3d`（亦支持 `cap.geohash_3d` 动态挂载）实现 21-bit×3 维 63-bit Morton Z-Order 空间填充曲线零堆交错编码/解码、归一化浮点三维量化与 WGS84 Haversine 球面大圆测距。

## 核心类型与 API

- `MortonError`: `CoordinateOutOfRange`
- `Point3D`: `{ x: u21, y: u21, z: u21 }`
- `mortonEncode3D(x, y, z) -> u64` / `mortonDecode3D(code) -> Point3D`
- `quantizeEncode3D(nx, ny, nz) -> MortonError!u64`
- `haversineMeters(lat1, lon1, lat2, lon2) -> f64`

## 示例

```cp
import { mortonEncode3D, mortonDecode3D, haversineMeters } from "cap:geo";

const code = mortonEncode3D(100, 200, 300);
const pt = mortonDecode3D(code);
```
