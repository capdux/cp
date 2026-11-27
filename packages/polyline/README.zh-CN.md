[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# polyline

Zero-heap Google Encoded Polyline (precision 5/6) GPS coordinate encoder and decoder for CP (compatible with @mapbox/polyline and Rust polyline)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:polyline` ([https://github.com/georust/polyline](https://github.com/georust/polyline))

| 元信息 | 值 |
| --- | --- |
| 包名 | `polyline` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add polyline
```

```cp
import {
    LatLng,
    precisionScale,
    roundToI64,
    encodeSigned,
    decodeSigned,
    encodePoints,
    decodePoints,
    countPoints,
} from "polyline";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `LatLng` | `class` | 字段: `lat: f64, lng: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `precisionScale` | `(precision: u32): f64` | 调用 `precisionScale` 并返回 `f64`。 |
| `roundToI64` | `(v: f64): i64` | 调用 `roundToI64` 并返回 `i64`。 |
| `encodeSigned` | `(val: i64, out: []u8): usize` | 通过 `encodeSigned` 编码或格式化输出，返回 `usize`。 |
| `decodeSigned` | `(data: string, start: usize, out_val: &mut i64): usize` | 通过 `decodeSigned` 解析输入数据，返回 `usize`。 |
| `encodePoints` | `(points: &[]LatLng, precision: u32, out: []u8): usize` | 通过 `encodePoints` 编码或格式化输出，返回 `usize`。 |
| `decodePoints` | `(encoded: string, precision: u32, out: []LatLng): usize` | 通过 `decodePoints` 解析输入数据，返回 `usize`。 |
| `countPoints` | `(encoded: string): usize` | 调用 `countPoints` 并返回 `usize`。 |
| `isValid` | `(encoded: string): bool` | 执行 `isValid` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { decodeSigned } from "polyline";

pub fn main() {
    let result = decodeSigned("example", 0, /* out_val: &mut i64 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
