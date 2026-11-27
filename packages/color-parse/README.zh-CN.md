[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# color-parse

Zero-allocation CSS Color Level 4 functional color parser (rgb/rgba/hsl/hsla with comma and space/slash syntax), formatter, and RGB <-> HSL converter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:color-parse` ([https://github.com/colorjs/color-parse](https://github.com/colorjs/color-parse)) / `crates.io:csscolorparser` ([https://github.com/mazznoer/csscolorparser-rs](https://github.com/mazznoer/csscolorparser-rs))（原库名 `color-parse` → CP 包名 `color-parse`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `color-parse` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add color-parse
```

```cp
import {
    CssRgba,
    CssHsla,
    ccAbsF64,
    ccClampF64,
    ccClampU8,
    ccNormHue,
    hslToRgb,
    rgbToHsl,
} from "color-parse";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CssRgba` | `class` | 字段: `r: u8, g: u8, b: u8, a: f64, valid: bool` |
| `CssHsla` | `class` | 字段: `h: f64, s: f64, l: f64, a: f64, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ccAbsF64` | `(x: f64): f64` | 调用 `ccAbsF64` 并返回 `f64`。 |
| `ccClampF64` | `(x: f64, lo: f64, hi: f64): f64` | 调用 `ccClampF64` 并返回 `f64`。 |
| `ccClampU8` | `(v: f64): u8` | 调用 `ccClampU8` 并返回 `u8`。 |
| `ccNormHue` | `(h: f64): f64` | 调用 `ccNormHue` 并返回 `f64`。 |
| `hslToRgb` | `(h: f64, s: f64, l: f64, a: f64): CssRgba` | 调用 `hslToRgb` 并返回 `CssRgba`。 |
| `rgbToHsl` | `(r: u8, g: u8, b: u8, a: f64): CssHsla` | 调用 `rgbToHsl` 并返回 `CssHsla`。 |
| `ccParseNum` | `(raw: &[]u8, out_val: &mut f64, is_pct: &mut bool, allow_deg: bool): bool` | 调用 `ccParseNum` 并返回 `bool`。 |
| `ccSplitArgs` | `(body: &[]u8, out_toks: []&[]u8): usize` | 调用 `ccSplitArgs` 并返回 `usize`。 |
| `parseCssColor` | `(input: string): CssRgba` | 通过 `parseCssColor` 解析输入数据，返回 `CssRgba`。 |
| `ccWriteRaw` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | 调用 `ccWriteRaw` 并返回 `bool`。 |
| `ccWriteU8` | `(out: []u8, pos: &mut usize, val: u8): bool` | 调用 `ccWriteU8` 并返回 `bool`。 |
| `ccWriteAlpha` | `(out: []u8, pos: &mut usize, a: f64): bool` | 调用 `ccWriteAlpha` 并返回 `bool`。 |
| `formatRgb` | `(c: CssRgba, out: []u8): usize` | 通过 `formatRgb` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseCssColor } from "color-parse";

pub fn main() {
    let result = parseCssColor("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
