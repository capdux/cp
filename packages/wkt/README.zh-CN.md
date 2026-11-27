[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# wkt

Zero-heap GIS Well-Known Text (WKT) parser, formatter, and bounding-box calculator for POINT, LINESTRING, POLYGON, and MULTIPOINT geometries (clean-room port of Rust wkt)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wkt` ([https://github.com/georust/wkt](https://github.com/georust/wkt))

| 元信息 | 值 |
| --- | --- |
| 包名 | `wkt` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add wkt
```

```cp
import {
    Coord2D,
    WktGeometry,
    WktBbox,
    wktZeroCoord,
    wktEmptyGeom,
    wktSkipWs,
    wktReadWord,
    wktParseNumber,
} from "wkt";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Coord2D` | `class` | 字段: `x: f64, y: f64` |
| `WktGeometry` | `class` | 字段: `kind: u8, is_empty: bool, coords: [32]Coord2D, coord_count: usize` |
| `WktBbox` | `class` | 字段: `min_x: f64, min_y: f64, max_x: f64, max_y: f64, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `wktZeroCoord` | `(): Coord2D` | 调用 `wktZeroCoord` 并返回 `Coord2D`。 |
| `wktEmptyGeom` | `(kind: u8): WktGeometry` | 调用 `wktEmptyGeom` 并返回 `WktGeometry`。 |
| `wktSkipWs` | `(s: string, pos: &mut usize)` | 就地执行 `wktSkipWs` 操作。 |
| `wktReadWord` | `(s: string, pos: &mut usize, out: []u8): usize` | 调用 `wktReadWord` 并返回 `usize`。 |
| `wktParseNumber` | `(s: string, pos: &mut usize, out: &mut f64): bool` | 调用 `wktParseNumber` 并返回 `bool`。 |
| `wktParseCoord` | `(s: string, pos: &mut usize, out: &mut Coord2D): bool` | 调用 `wktParseCoord` 并返回 `bool`。 |
| `wktConsumeChar` | `(s: string, pos: &mut usize, ch: u8): bool` | 调用 `wktConsumeChar` 并返回 `bool`。 |
| `wktWriteF64` | `(out: []u8, pos: usize, val: f64): usize` | 调用 `wktWriteF64` 并返回 `usize`。 |
| `wktWriteCoord` | `(out: []u8, pos: usize, c: Coord2D): usize` | 调用 `wktWriteCoord` 并返回 `usize`。 |
| `parseWkt` | `(input: string): WktGeometry?` | 通过 `parseWkt` 解析输入数据，返回 `WktGeometry?`。 |
| `formatWkt` | `(g: WktGeometry, out: []u8): usize` | 通过 `formatWkt` 编码或格式化输出，返回 `usize`。 |
| `bboxWkt` | `(g: WktGeometry): WktBbox` | 调用 `bboxWkt` 并返回 `WktBbox`。 |

## 最小调用示例

```cp
import { parseWkt } from "wkt";

pub fn main() {
    let result = parseWkt("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
