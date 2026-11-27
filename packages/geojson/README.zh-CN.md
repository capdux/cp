[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# geojson

Zero-heap RFC 7946 GeoJSON geometry & feature parser, formatter, and WKT interop built on wkt (clean-room port of Rust geojson)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:geojson` ([https://github.com/georust/geojson](https://github.com/georust/geojson))

| 元信息 | 值 |
| --- | --- |
| 包名 | `geojson` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add geojson
```

```cp
import {
    GeoJsonGeometry,
    GeoJsonFeature,
    gjZeroCoord,
    gjEmptyGeom,
    gjIsSpace,
    gjSkipWs,
    gjConsumeChar,
    gjWriteByte,
} from "geojson";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GeoJsonGeometry` | `class` | 字段: `kind: u8, coords: [32]Coord2D, coord_count: usize` · 方法: `fromWkt, toWkt` |
| `GeoJsonFeature` | `class` | 字段: `id: string, geom: WktGeometry, has_bbox: bool, bbox: WktBbox` · 方法: `init` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `gjZeroCoord` | `(): Coord2D` | 调用 `gjZeroCoord` 并返回 `Coord2D`。 |
| `gjEmptyGeom` | `(kind: u8): WktGeometry` | 调用 `gjEmptyGeom` 并返回 `WktGeometry`。 |
| `gjIsSpace` | `(c: u8): bool` | 调用 `gjIsSpace` 并返回 `bool`。 |
| `gjSkipWs` | `(s: string, pos: &mut usize)` | 就地执行 `gjSkipWs` 操作。 |
| `gjConsumeChar` | `(s: string, pos: &mut usize, ch: u8): bool` | 调用 `gjConsumeChar` 并返回 `bool`。 |
| `gjWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `gjWriteByte` 并返回 `usize`。 |
| `gjWriteJsonEscaped` | `(out: []u8, pos: usize, s: string): usize` | 调用 `gjWriteJsonEscaped` 并返回 `usize`。 |
| `gjWriteF64` | `(out: []u8, pos: usize, val: f64): usize` | 调用 `gjWriteF64` 并返回 `usize`。 |
| `gjWriteCoord` | `(out: []u8, pos: usize, c: Coord2D): usize` | 调用 `gjWriteCoord` 并返回 `usize`。 |
| `gjParseNumber` | `(s: string, pos: &mut usize, out: &mut f64): bool` | 调用 `gjParseNumber` 并返回 `bool`。 |
| `gjParseStringSlice` | `(s: string, pos: &mut usize, start_out: &mut usize, end_out: &mut usize): bool` | 调用 `gjParseStringSlice` 并返回 `bool`。 |
| `gjSkipValue` | `(s: string, pos: &mut usize, depth: u32): bool` | 调用 `gjSkipValue` 并返回 `bool`。 |
| `gjParsePosition` | `(s: string, pos: &mut usize, out: &mut Coord2D): bool` | 调用 `gjParsePosition` 并返回 `bool`。 |
| `gjParsePosList` | `(s: string, pos: &mut usize, geom: &mut WktGeometry): bool` | 调用 `gjParsePosList` 并返回 `bool`。 |
| `formatGeoJsonGeometry` | `(g: WktGeometry, out: []u8): usize` | 通过 `formatGeoJsonGeometry` 编码或格式化输出，返回 `usize`。 |
| `formatGeoJsonFeature` | `(f: GeoJsonFeature, out: []u8): usize` | 通过 `formatGeoJsonFeature` 编码或格式化输出，返回 `usize`。 |
| `parseGeoJsonGeometry` | `(json: string): WktGeometry?` | 通过 `parseGeoJsonGeometry` 解析输入数据，返回 `WktGeometry?`。 |
| `parseGeoJsonPoint` | `(json: string): WktGeometry?` | 通过 `parseGeoJsonPoint` 解析输入数据，返回 `WktGeometry?`。 |
| `wktToGeoJson` | `(wkt_str: string, out: []u8): usize` | 调用 `wktToGeoJson` 并返回 `usize`。 |
| `geoJsonToWkt` | `(json: string, out: []u8): usize` | 调用 `geoJsonToWkt` 并返回 `usize`。 |

## 最小调用示例

```cp
import { parseGeoJsonGeometry } from "geojson";

pub fn main() {
    let result = parseGeoJsonGeometry("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
