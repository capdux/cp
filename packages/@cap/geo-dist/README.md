# @cap/geo-dist

Zero-allocation WGS-84 spherical geometry engine for CP — clean-room MIT parity with Rust `geo` / `haversine` and npm `geolib`.

纯 CP 零堆分配球面地理计算引擎（WGS-84 平均地球半径 `6371008.8m`），零外部依赖内置高精度三角函数，支持 Haversine 大圆距离、初始方位角、目标点推算、球面中点、半径包围盒与射线法多边形内外判定。

## Install / 安装

```bash
cap add @cap/geo-dist
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Coord` | `struct { lat: f64, lon: f64 }` | Geographic coordinate in decimal degrees |
| `Bounds` | `struct { min_lat, max_lat, min_lon, max_lon }` | Geographic bounding box |
| `haversineMeters` | `(a: Coord, b: Coord) -> f64` | Great-circle distance in meters |
| `initialBearing` | `(a: Coord, b: Coord) -> f64` | Initial compass bearing in `[0.0, 360.0)` degrees |
| `destinationPoint` | `(origin: Coord, bearing_deg: f64, distance_m: f64) -> Coord` | Destination coordinate along bearing & distance |
| `midpoint` | `(a: Coord, b: Coord) -> Coord` | Great-circle spherical midpoint |
| `boundingBox` | `(center: Coord, radius_m: f64) -> Bounds` | Bounding box for given center & radius in meters |
| `pointInPolygon` | `(pt: Coord, poly: []const Coord) -> bool` | Even-odd ray-casting point-in-polygon check |

## Usage / 使用示例

```cp
import { Coord, haversineMeters, initialBearing, destinationPoint } from "@cap/geo-dist";

const nyc = Coord{ lat: 40.7128, lon: -74.0060 };
const lon = Coord{ lat: 51.5074, lon: -0.1278 };
const dist = haversineMeters(nyc, lon); // ~5,570,230 m
```
