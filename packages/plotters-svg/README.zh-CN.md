[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# plotters-svg

Zero-heap pure CP SVG vector graphics drawing backend (plotters-svg parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:plotters-svg` ([https://github.com/plotters-rs/plotters](https://github.com/plotters-rs/plotters)) — `MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `plotters-svg` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add plotters-svg
```

```cp
import {
    SvgColor,
    SvgCanvas,
    psvgWriteByte,
    psvgWriteStr,
    psvgWriteU32,
    psvgWriteI32,
    psvgWriteRgb,
    psvgWriteXmlEscaped,
} from "plotters-svg";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SvgColor` | `class` | 字段: `r: u8, g: u8, b: u8` · 方法: `rgb, black, white, red, green, blue` |
| `SvgCanvas` | `class` | 字段: `buf: []u8, pos: usize, overflowed: bool` · 方法: `new, written, hasOverflow, slice, begin, end, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `psvgWriteByte` | `(buf: []u8, pos: usize, b: u8): usize` | 调用 `psvgWriteByte` 并返回 `usize`。 |
| `psvgWriteStr` | `(buf: []u8, pos: usize, s: string): usize` | 调用 `psvgWriteStr` 并返回 `usize`。 |
| `psvgWriteU32` | `(buf: []u8, pos: usize, val: u32): usize` | 调用 `psvgWriteU32` 并返回 `usize`。 |
| `psvgWriteI32` | `(buf: []u8, pos: usize, val: i32): usize` | 调用 `psvgWriteI32` 并返回 `usize`。 |
| `psvgWriteRgb` | `(buf: []u8, pos: usize, c: SvgColor): usize` | 调用 `psvgWriteRgb` 并返回 `usize`。 |
| `psvgWriteXmlEscaped` | `(buf: []u8, pos: usize, text: string): usize` | 调用 `psvgWriteXmlEscaped` 并返回 `usize`。 |

## 最小调用示例

```cp
import { psvgWriteStr } from "plotters-svg";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = psvgWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
