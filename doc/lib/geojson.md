# CP 标准库：RFC 7946 GeoJSON 编解码、地理围栏与轨迹抽稀引擎手册 (`cap:geojson`)

## 一、 架构总览与设计准则

`cap:geojson` 是 CP 语言官方内置的纯 CP、零 C/POSIX 依赖的 **RFC 7946 GeoJSON 解析序列化、包围盒计算、含孔洞多边形地理围栏判定与 Douglas-Peucker 折线抽稀引擎**（基于 `std.json` 构建）。既支持 `import { ... } from "cap:geojson"` 静态导入，也支持 `import cap from "cap"; cap.geojson.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/geojson/geojson.cp` | `// Source: cp/lib/geojson/geojson.cp (RFC 7946 GeoJSON Codec, Geofence & Simplifier)` | `GeoJsonError`、`Position`、`BBox`、`PolygonData`、`Geometry`、`Feature`、`FeatureCollection`、`GeoJsonDoc`、`parse`、`stringify`、`computeBBox`、`pointInPolygon`、`simplifyDouglasPeucker` |
| `lib/geojson/index.cp` | `// Source: cp/lib/geojson/index.cp (Unified Exports for cap:geojson)` | `cap:geojson` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与基础几何类型

- **`GeoJsonError`**：
  - `InvalidJson`：底层 JSON 语法非法。
  - `InvalidType`：缺少 `"type"` 字段或 `"type"` 非 RFC 7946 合法 GeoJSON 类型。
  - `InvalidCoordinates`：坐标数组少于 2 个元素、类型非数值或包含非有限浮点数（`NaN`/`Inf`）。
  - `InvalidRing`：Polygon 环少于 4 个点，或首尾坐标未闭合。
  - `OutOfMemory`：底层内存分配失败。
- **`Position`**：`{ lon: f64, lat: f64, alt: f64? }`
- **`BBox`**：`{ min_lon: f64, min_lat: f64, max_lon: f64, max_lat: f64 }`
- **`PolygonData`**：`{ rings: [][]Position }`（`rings[0]` 为外环，`rings[1..]` 为内环孔洞；含 `deinit(*_, allocator)`）。

### 2. Tagged Union 与文档模型

- **`Geometry`**：`point: Position`、`multi_point: []Position`、`line_string: []Position`、`multi_line_string: [][]Position`、`polygon: PolygonData`、`multi_polygon: []PolygonData`、`geometry_collection: []Geometry`（含递归 `deinit(*_, allocator)`）。
- **`Feature`**：`{ id: ([]const u8)?, geometry: Geometry?, properties_json: []const u8 }`（含 `deinit(*_, allocator)`）。
- **`FeatureCollection`**：`{ features: []Feature }`（含 `deinit(*_, allocator)`）。
- **`GeoJsonDoc`**：`geometry: Geometry`、`feature: Feature`、`feature_collection: FeatureCollection`（含 `deinit(*_, allocator)`）。

### 3. 编解码、围栏与抽稀接口

- **`parse(allocator: std.mem.Allocator, json_str: []const u8): GeoJsonDoc ! GeoJsonError`**：
  严格解析 RFC 7946 JSON 文本，校验 `"type"`、坐标有限性与至少 2 维、Polygon 环至少 4 点且首尾闭合。
- **`stringify(allocator: std.mem.Allocator, doc: GeoJsonDoc): ([]u8) ! GeoJsonError`**：
  将 `GeoJsonDoc` 规范化序列化为紧凑 GeoJSON 字符串，并在非空包围盒存在时自动输出 `"bbox"` 字段。
- **`computeBBox(geom: Geometry): BBox?`** / **`computeDocBBox(doc: GeoJsonDoc): BBox?`**：
  计算几何体或文档的 2D 最小外接矩形（空几何体返回 `null`）。
- **`pointInPolygon(pt: Position, poly: PolygonData): bool`**：
  基于 Ray-Casting 射线法与边界线段高精度判定，精确支持外环边界命中及内环孔洞挖空排除（位于孔洞内部返回 `false`）。
- **`simplifyDouglasPeucker(allocator: std.mem.Allocator, pts: []const Position, epsilon: f64): ([]Position) ! GeoJsonError`**：
  迭代式经典 Douglas-Peucker 折线抽稀算法，严格保留起终点与垂直距离超过 `epsilon` 的关键拐点。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { parse, stringify, computeBBox, pointInPolygon, simplifyDouglasPeucker, Position } from "cap:geojson";
import cap from "cap";

pub fn demoGeoJson(): ! {
    const alloc = std.heap.page_allocator;
    const raw = "{\"type\":\"Polygon\",\"coordinates\":[[[0,0],[10,0],[10,10],[0,10],[0,0]],[[2,2],[8,2],[8,8],[2,8],[2,2]]]}";

    let doc = parse(alloc, raw)?;
    defer doc.deinit(alloc);

    const out = stringify(alloc, doc)?;
    defer alloc.free(out);

    switch (doc) {
        .geometry => |g| {
            const bbox = computeBBox(g).?;
            * = bbox.max_lon;
            switch (g) {
                .polygon => |poly| {
                    const inside_shell = pointInPolygon(Position{ lon: 1.0, lat: 1.0, alt: null }, poly);
                    const inside_hole = pointInPolygon(Position{ lon: 5.0, lat: 5.0, alt: null }, poly);
                    * = inside_shell; // true
                    * = inside_hole;  // false
                },
                else => {},
            }
        },
        else => {},
    }

    let pts: [3]Position = .{
        Position{ lon: 0.0, lat: 0.0, alt: null },
        Position{ lon: 1.0, lat: 0.05, alt: null },
        Position{ lon: 2.0, lat: 0.0, alt: null },
    };
    const simplified = cap.geojson.simplifyDouglasPeucker(alloc, &pts, 0.1)?;
    defer alloc.free(simplified);
}
```
