[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# geo

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation spherical geometry engine: Haversine distance, initial bearing, destination point, midpoint, bounding box, and point-in-polygon (Rust geo / haversine / geolib parity) for CP

## 安装与导入

```bash
cap add geo
```

```cp
import { haversineMeters } from "geo";
```

## 核心导出 API

- `export fn haversineMeters(a: Coord, b: Coord): f64;`
- `export fn initialBearing(a: Coord, b: Coord): f64;`
- `export fn destinationPoint(origin: Coord, bearing_deg: f64, distance_m: f64): Coord;`
- `export fn midpoint(a: Coord, b: Coord): Coord;`
- `export fn boundingBox(center: Coord, radius_m: f64): Bounds;`
- `export fn pointInPolygon(pt: Coord, poly: &[]Coord): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`geo@1.0.0`)，许可证：MIT
