# CP 标准库：地理空间计算与 Geohash 邻近检索引擎 (`cap:geo` / `cap.geo`)

## 一、 架构总览

`cap:geo` 是 CP 语言官方内置的纯原生地理空间计算与 Geohash 邻近检索标准库，专为 LBS 位置服务、地理围栏（Geofencing）、网格索引与半径/最近邻查询打造。全模块零 C 依赖、零 `std.posix`、零 `@import("bun")`，支持 `import { ... } from "cap:geo"` 与 `import cap from "cap"; cap.geo.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/geo/coord.cp` | `// Source: cp/lib/geo/coord.cp (Geodesic Haversine, Bearing, BBox & Point-in-Polygon)` | `EARTH_RADIUS_M`、`GeoError`、`LatLng`、`BBox`、`Geo`（Haversine/方位角/目标点投影/射线法多边形判定） |
| `lib/geo/geohash.cp` | `// Source: cp/lib/geo/geohash.cp (Base32 Geohash Encoder, Decoder & 8-Neighbor Lookup)` | `GeohashDecoded`、`GeohashNeighbors`、`Geohash`（1..12 位 Base32 编解码与 8 邻域格网计算） |
| `lib/geo/spatial.cp` | `// Source: cp/lib/geo/spatial.cp (In-Memory Radius & Nearest-Neighbor Spatial Index)` | `GeoPointEntry`、`SpatialIndex`（O(1) 更新/删除、纬度剪枝半径查询与 Nearest-K 排序） |
| `lib/geo/index.cp` | `// Source: cp/lib/geo/index.cp (Unified Exports for cap:geo)` | `cap:geo` 与 `cap.geo` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. 坐标、包围盒与大地测量（`coord.cp`）

- **`EARTH_RADIUS_M: f64 = 6371008.8`**（IUGG 地球平均半径，单位：米）
- **`GeoError`**：`InvalidCoordinate`、`InvalidGeohash`、`InvalidPrecision`、`OutOfMemory`
- **`LatLng`**：
  - 字段：`lat: f64`、`lng: f64`
  - 方法：`init(lat, lng): LatLng ! GeoError`（严格校验非 NaN/Inf 且 `lat ∈ [-90, 90]`、`lng ∈ [-180, 180]`）、`isValid(): bool`、`approxEql(other, eps_deg = 0.000001): bool`
- **`BBox`**：
  - 字段：`min_lat: f64`、`max_lat: f64`、`min_lng: f64`、`max_lng: f64`
  - 方法：`init(min_lat, max_lat, min_lng, max_lng): BBox ! GeoError`、`fromPoint(pt): BBox ! GeoError`、`contains(pt): bool`、`intersects(other): bool`、`expand(*_, pt)`、`center(): LatLng`
- **`Geo`**：
  - `haversineMeters(a, b): f64`：球面 Haversine 大圆距离（米）
  - `initialBearingDeg(a, b): f64`：初始前向方位角，归一化至 `[0.0, 360.0)`
  - `destinationPoint(origin, bearing_deg, dist_m): LatLng`：沿指定方位角与距离投影目标坐标（自动环绕经度至 `[-180, 180]`）
  - `pointInPolygon(pt, ring): bool`：射线法 + 边界线段容差地理围栏判定（支持凹凸多边形与闭合/非闭合顶点环）

### 2. Base32 Geohash 网格引擎（`geohash.cp`）

- **`GeohashDecoded`**：`center: LatLng`、`bbox: BBox`、`lat_err: f64`、`lng_err: f64`
- **`GeohashNeighbors`**：`n`、`ne`、`e`、`se`、`s`、`sw`、`w`、`nw`（`[]u8`，调用 `deinit(alloc)` 释放）
- **`Geohash`**：
  - `encode(alloc, pt, precision: u8): ([]u8) ! GeoError`（支持 `1..12` 位标准 Base32 `0123456789bcdefghjkmnpqrstuvwxyz`）
  - `decode(hash: string): GeohashDecoded ! GeoError`
  - `neighbors(alloc, hash: string): GeohashNeighbors ! GeoError`（无浮点漂移查表法精确计算全部 8 方向相邻格网）

### 3. 内存空间索引（`spatial.cp`）

- **`GeoPointEntry`**：`id: string`、`coord: LatLng`、`distance_m: f64`（`id` 借用自 `SpatialIndex`）
- **`SpatialIndex`**：
  - 生命周期：`init(alloc): SpatialIndex`、`deinit(*_)`、`len(): usize`
  - 变更：`upsert(*_, id, pt): void ! GeoError`（内部深拷贝 `id`）、`remove(*_, id): bool`（O(1) swap-remove）
  - 查询：`withinRadius(alloc, center, radius_m): ([]GeoPointEntry) ! GeoError`（纬度剪枝 + Haversine 距离升序）、`nearestK(alloc, center, k): ([]GeoPointEntry) ! GeoError`（返回切片由调用方 `alloc.free` 释放）

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { LatLng, Geo, Geohash, SpatialIndex } from "cap:geo";

pub fn demoGeo(): !void {
    const alloc = std.heap.page_allocator;
    const beijing = LatLng.init(39.9042, 116.4074)?;
    const shanghai = LatLng.init(31.2304, 121.4737)?;

    const dist_m = Geo.haversineMeters(beijing, shanghai);
    const hash = Geohash.encode(alloc, beijing, 6)?; // "wx4g0b"
    defer alloc.free(hash);

    let nb = Geohash.neighbors(alloc, hash)?;
    defer nb.deinit(alloc);

    let idx = cap.geo.SpatialIndex.init(alloc);
    defer idx.deinit();
    idx.upsert("shanghai", shanghai)?;
    const hits = idx.withinRadius(alloc, beijing, dist_m + 1000.0)?;
    defer alloc.free(hits);
}
```

---

## 扩展 GIS 格式、导航协议与空间计算模块（零冲突归并）

以下 12 个纯 CP 地理空间/GIS/导航/空间索引子模块已统一收敛至 `lib/geo/` 并零冲突导出至 `cap:geo`：

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/geo/geojson.cp` | RFC 7946 GeoJSON 编解码与折线简化 | `parse`, `stringify`, `computeBBox`, `computeDocBBox`, `pointInPolygon`, `simplifyDouglasPeucker`, `GeoJsonBBox`, `FeatureCollection` |
| `lib/geo/topojson.cp` | TopoJSON v1.0 量化弧与拓扑解析 | `isTopoJson`, `parseTopoHeader`, `TopoArcIter`, `TopoObjectIter`, `resolveArcRef`, `TopoTransform` |
| `lib/geo/wkt.cp` | OGC WKT / WKB 几何编解码 | `parseWkt`, `formatWkt`, `encodeWkb`, `decodeWkb`, `WktGeometry`, `WktCoord`, `WktError` |
| `lib/geo/wkb.cp` | 零分配 OGC WKB / PostGIS EWKB | `parseWkbHeader`, `wkbByteLength`, `WkbReader`, `WkbWriter`, `WkbPointIter`, `WkbGeomType` |
| `lib/geo/gpx.cp` | GPX 1.0/1.1 轨迹 XML 解析与统计 | `isGpx`, `parseGpxHeader`, `GpxWaypointIter`, `GpxTrackPointIter`, `summarizeGpx`, `haversineMeters` |
| `lib/geo/kml.cp` | OGC KML 2.2 地标与坐标解析 | `isKml`, `summarizeKml`, `KmlPlacemarkIter`, `KmlCoordIter`, `KmlGeomKind` |
| `lib/geo/nmea.cp` | NMEA 0183 GNSS 卫星定位报文 | `parseNmeaSentence`, `verifyNmeaSentence`, `parseNmeaGga`, `parseNmeaRmc`, `parseNmeaVtg`, `NmeaLineIter` |
| `lib/geo/olc.cp` | Open Location Code (Plus Codes) | `olcEncode`, `olcDecode`, `olcShorten`, `olcRecoverNearest`, `isValidOlc`, `OlcArea` |
| `lib/geo/ais.cp` | ITU-R M.1371 海事 AIS 船舶报文 | `parseAisSentence`, `parseAisPositionReport`, `parseAisClassBPosition`, `aisMessageType`, `AisBitReader` |
| `lib/geo/geohash_3d.cp` | 3D 莫顿空间编码 | `mortonEncode3D`, `mortonDecode3D`, `quantizeEncode3D`, `Point3D`, `MortonError` |
| `lib/geo/geom2d.cp` | 2D 计算几何、凸包与碰撞检测 | `Geom2D`, `Point2`, `Segment2`, `Aabb2`, `Circle2`, `Orientation`, `GeomError` |
| `lib/geo/geo_quadtree.cp` | 泛型四叉树 2D 空间索引 | `QuadTree`, `PointItem`, `Rect`, `QuadError` |

> **重叠对照参考归档说明**：`lib/geo/geohash_.cp` 为与 `lib/geo/geohash.cp` 功能重叠的对照参考副本（文件首行带 `// Reference Only:` 声明），不在 `lib/geo/index.cp` 冲突导出。
