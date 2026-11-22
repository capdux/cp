# `cap:topojson` — 零分配 TopoJSON v1.0 拓扑压缩几何解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:geo`，导入请使用 `import { ... } from "cap:geo"`。

> **模块路径**：`lib/topojson/`（入口 `lib/topojson/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，支持 TopoJSON v1.0 `Topology` 校验、`transform` 量化还原、`arcs` 差分编码解码、带符号 Arc 反向索引解析与 `objects` 零拷贝迭代。

---

## 一、导入方式

```cp
import {
    TopoTransform,
    TopoPoint,
    TopoBBox,
    TopoArcRef,
    TopoHeader,
    TopoObjectEntry,
    TopoArcIter,
    TopoArcPointIter,
    TopoObjectIter,
    isTopoJson,
    parseTopoHeader,
    resolveArcRef,
    arcSliceAt,
    countArcPoints,
} from "cap:geo";
```

---

## 二、核心数据结构

| 类型 | 字段 | 说明 |
| :--- | :--- | :--- |
| `TopoTransform` | `scale_x: f64`, `scale_y: f64`, `translate_x: f64`, `translate_y: f64`, `present: bool` | TopoJSON 仿射量化参数；提供 `apply(raw_x, raw_y): TopoPoint` 方法 |
| `TopoPoint` | `x: f64`, `y: f64` | 解码后的二维坐标点（通常为经度、纬度） |
| `TopoBBox` | `min_x: f64`, `min_y: f64`, `max_x: f64`, `max_y: f64`, `present: bool` | 可选边界框（支持 4 元 2D 与 6 元 3D 数组提取） |
| `TopoArcRef` | `index: usize`, `reversed: bool` | 解析后的带符号 Arc 引用（0 起始下标及是否反向遍历） |
| `TopoHeader` | `transform: TopoTransform`, `bbox: TopoBBox`, `arcs_slice: []const u8`, `objects_slice: []const u8`, `arc_count: usize` | 零拷贝 Topology 头部视图；提供 `hasTransform()`, `hasBBox()`, `arcs()`, `objects()` |
| `TopoObjectEntry` | `name: []const u8`, `geom_type: []const u8`, `raw_json: []const u8` | `"objects"` 中单个命名几何对象的零拷贝视图 |

---

## 三、核心 API

### 1. 校验与头部解析
- `isTopoJson(data: []const u8): bool`  
  校验 `data` 是否为完整闭合的顶层 JSON 对象且包含 `"type": "Topology"`（支持任意空白与字段顺序）。
- `parseTopoHeader(data: []const u8): TopoHeader?`  
  零拷贝提取可选 `transform`、可选 `bbox`、顶层 `"arcs"` 数组切片、`"objects"` 对象切片及 `arc_count`。非 Topology 或截断/畸形 JSON 返回 `null`。

### 2. Arc 迭代与差分坐标解码
- `TopoArcIter.init(arcs_slice: []const u8): TopoArcIter` / `next(*_): ([]const u8)?`  
  顺序迭代 `"arcs"` 数组中的每条 Arc 原始切片。
- `arcSliceAt(header: TopoHeader, idx: usize): ([]const u8)?`  
  按 0 起始下标获取第 `idx` 条 Arc 切片；越界返回 `null`。
- `TopoArcPointIter.init(arc_slice: []const u8, transform: TopoTransform): TopoArcPointIter` / `next(*_): TopoPoint?`  
  - 当 `transform.present == true`：按 TopoJSON v1.0 规范对 `[[dx0, dy0], [dx1, dy1], ...]` 执行差分累加并量化还原：`x = acc_x * scale_x + translate_x`、`y = acc_y * scale_y + translate_y`。
  - 当 `transform.present == false`：直接解码原始浮点坐标 `[x, y]`。
- `countArcPoints(arc_slice: []const u8): usize`  
  零分配统计单条 Arc 中的坐标点数量。
- `resolveArcRef(index: i64): TopoArcRef`  
  按 TopoJSON 规范解码带符号 Arc 索引：`index >= 0` 为正向 `index`；`index < 0` 为反向按位取反 `~index`（即 `-(index + 1)`）。

### 3. Objects 零拷贝迭代
- `TopoObjectIter.init(objects_slice: []const u8): TopoObjectIter` / `next(*_): TopoObjectEntry?`  
  零拷贝遍历 `"objects"` 下的所有命名几何对象，提取对象名 `name`、几何类型 `geom_type` 与原始 JSON 切片 `raw_json`。

---

## 四、使用示例

```cp
import {
    parseTopoHeader,
    arcSliceAt,
    TopoArcPointIter,
    resolveArcRef,
} from "cap:geo";

const raw =
    "{\"type\":\"Topology\"," ++
    "\"transform\":{\"scale\":[0.001,0.001],\"translate\":[120.0,30.0]}," ++
    "\"objects\":{\"zone\":{\"type\":\"Polygon\",\"arcs\":[[0,-1]]}}," ++
    "\"arcs\":[[[1000,2000],[500,-1000],[250,500]]]}";

if (hdr := parseTopoHeader(raw)) {
    if (arc0 := arcSliceAt(hdr, 0)) {
        let pt_it = TopoArcPointIter.init(arc0, hdr.transform);
        while (pt := pt_it.next()) {
            // (121.0, 32.0) -> (121.5, 31.0) -> (121.75, 31.5)
            * = pt;
        }
    }
    const rev = resolveArcRef(-1); // index = 0, reversed = true
    * = rev;
}
```
