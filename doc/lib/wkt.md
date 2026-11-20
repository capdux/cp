# CP 标准库：OGC WKT & ISO WKB 几何编解码引擎手册 (`cap:wkt`)

## 一、 架构总览与设计准则

`cap:wkt` 是 CP 语言官方内置的纯 CP、零 C/POSIX 依赖的 **OGC Well-Known Text (WKT) 解析/格式化与 ISO Well-Known Binary (WKB) 双向二进制编解码引擎**。既支持 `import { ... } from "cap:wkt"` 静态导入，也支持 `import cap from "cap"; cap.wkt.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/wkt/wkt.cp` | `// Source: cp/lib/wkt/wkt.cp (OGC Well-Known Text WKT & Well-Known Binary WKB Geometry Codec)` | `WktError`、`WkbByteOrder`、`WktCoord`、`WktPolygon`、`WktGeometry`、`parseWkt`、`formatWkt`、`encodeWkb`、`decodeWkb` |
| `lib/wkt/index.cp` | `// Source: cp/lib/wkt/index.cp (Unified Exports for cap:wkt)` | `cap:wkt` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与基础几何类型

- **`WktError`**：
  - `UnexpectedToken`：WKT 语法中遇到非预期字符或尾部多余字符。
  - `UnexpectedEof`：WKT 文本或 WKB 字节流提前截断。
  - `InvalidGeometryType`：非法几何类型名称或 WKB 类型编号不在 ISO `1..7`（含 Z/M/ZM 偏移）范围内。
  - `InvalidCoordinate`：坐标数字非法、维度缺失（<2 个数字）或包含 `NaN`/`Inf`。
  - `InvalidRing`：Polygon 环少于 4 个点或首尾坐标未闭合。
  - `InvalidWkbHeader`：WKB 字节序标记非 `0x00`/`0x01` 或存在非法尾随字节。
  - `MaxDepthExceeded`：`GEOMETRYCOLLECTION` 嵌套深度超过 32 层上限。
  - `OutOfMemory`：底层内存分配失败。
- **`WkbByteOrder`**：`BigEndian = 0`、`LittleEndian = 1`（`u8` 枚举）。
- **`WktCoord`**：`{ x: f64, y: f64, z: f64?, m: f64? }`
- **`WktPolygon`**：`{ rings: [][]WktCoord }`（`rings[0]` 为外环，`rings[1..]` 为内环孔洞；含 `deinit(*_, allocator)`）。

### 2. Tagged Union 几何模型

- **`WktGeometry`**：
  - `point: WktCoord?`（`null` 表示 `POINT EMPTY`）
  - `line_string: []WktCoord`
  - `polygon: WktPolygon`
  - `multi_point: []WktCoord?`
  - `multi_line_string: [][]WktCoord`
  - `multi_polygon: []WktPolygon`
  - `geometry_collection: []WktGeometry`
  - 含递归释放方法 `deinit(*_, allocator: std.mem.Allocator)`。

### 3. WKT / WKB 编解码接口

- **`parseWkt(allocator: std.mem.Allocator, text: []const u8): WktGeometry ! WktError`**：
  大小写不敏感解析 `POINT`、`LINESTRING`、`POLYGON`、`MULTIPOINT`（同时兼容 `((x y), ...)` 与 `(x y, ...)` 两种写法）、`MULTILINESTRING`、`MULTIPOLYGON`、`GEOMETRYCOLLECTION` 与 `EMPTY` 关键字，支持 `Z`/`M`/`ZM` 维度修饰符，内置 32 层嵌套深度上限保护。
- **`formatWkt(allocator: std.mem.Allocator, geom: WktGeometry): ([]u8) ! WktError`**：
  将 `WktGeometry` 格式化为规范化大写 OGC WKT 字符串。
- **`encodeWkb(allocator: std.mem.Allocator, geom: WktGeometry, byte_order: WkbByteOrder): ([]u8) ! WktError`**：
  将 `WktGeometry` 编码为指定字节序（`BigEndian` `0x00` 或 `LittleEndian` `0x01`）的 ISO WKB 字节流。
- **`decodeWkb(allocator: std.mem.Allocator, wkb_bytes: []const u8): WktGeometry ! WktError`**：
  严格校验 WKB 字节序标记、类型编号、截断长度、Polygon 环闭合性与 32 层嵌套上限并解码为 `WktGeometry`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { parseWkt, formatWkt, encodeWkb, decodeWkb, WkbByteOrder } from "cap:wkt";
import cap from "cap";

pub fn demoWkt(): ! {
    const alloc = std.heap.page_allocator;
    const raw = "polygon ((0 0, 10 0, 10 10, 0 10, 0 0), (2 2, 8 2, 8 8, 2 8, 2 2))";

    let geom = parseWkt(alloc, raw)?;
    defer geom.deinit(alloc);

    const wkb_le = encodeWkb(alloc, geom, WkbByteOrder.LittleEndian)?;
    defer alloc.free(wkb_le);

    let decoded = cap.wkt.decodeWkb(alloc, wkb_le)?;
    defer decoded.deinit(alloc);

    const canonical = formatWkt(alloc, decoded)?;
    defer alloc.free(canonical);
}
```
