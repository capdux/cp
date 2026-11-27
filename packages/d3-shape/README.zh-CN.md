[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# d3-shape

Pure CP SVG shape and pie chart path generator (line, step, arc, pie)

| 元信息 | 值 |
| --- | --- |
| 包名 | `d3-shape` |
| 版本 | `3.2.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add d3-shape
```

```cp
import {
    Point2D,
    PieSlice,
    d3shWriteByte,
    d3shWriteStr,
    d3shWriteU64,
    d3shWriteFloat,
    d3shWritePair,
    computePie,
} from "d3-shape";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Point2D` | `class` | 字段: `x: f64, y: f64` |
| `PieSlice` | `class` | 字段: `value: f64, index: usize, start_angle: f64, end_angle: f64, pad_angle: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `d3shWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `d3shWriteByte` 并返回 `usize`。 |
| `d3shWriteStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `d3shWriteStr` 并返回 `usize`。 |
| `d3shWriteU64` | `(out: []u8, pos: usize, val: u64): usize` | 调用 `d3shWriteU64` 并返回 `usize`。 |
| `d3shWriteFloat` | `(out: []u8, pos: usize, val: f64): usize` | 调用 `d3shWriteFloat` 并返回 `usize`。 |
| `d3shWritePair` | `(out: []u8, pos: usize, x: f64, y: f64): usize` | 调用 `d3shWritePair` 并返回 `usize`。 |
| `computePie` | `(values: &[]f64, start_angle: f64, end_angle: f64, pad_angle: f64, out: []PieSlice): usize` | 调用 `computePie` 并返回 `usize`。 |
| `linePath` | `(pts: &[]Point2D, out: []u8): usize` | 调用 `linePath` 并返回 `usize`。 |
| `stepLinePath` | `(pts: &[]Point2D, out: []u8): usize` | 调用 `stepLinePath` 并返回 `usize`。 |
| `arcPath` | `(inner_r: f64, outer_r: f64, start_angle: f64, end_angle: f64, out: []u8): usize` | 调用 `arcPath` 并返回 `usize`。 |

## 最小调用示例

```cp
import { d3shWriteStr } from "d3-shape";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = d3shWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
