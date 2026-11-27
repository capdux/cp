[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# haversine

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation WGS-84 spherical Haversine great-circle distance, initial bearing, midpoint, and destination point calculator for CP.

## 安装与导入

```bash
cap add haversine
```

```cp
import { isValidCoord } from "haversine";
```

## 核心导出 API

- `export fn isValidCoord(p: LatLon): bool;`
- `export fn distanceMeters(a: LatLon, b: LatLon): f64;`
- `export fn distanceKm(a: LatLon, b: LatLon): f64;`
- `export fn distanceMiles(a: LatLon, b: LatLon): f64;`
- `export fn initialBearing(a: LatLon, b: LatLon): f64;`
- `export fn midpoint(a: LatLon, b: LatLon): LatLon;`
- `export fn destinationPoint(start: LatLon, bearing_deg: f64, distance_m: f64): LatLon;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`haversine@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`haversine@1.0.0`)，许可证：MIT
