[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# color-convert

Zero-allocation color space conversions (RGB, HSV, HWB, HSL, CMYK, ANSI256, ANSI16, Hex, CSS keywords) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`color-convert`](https://github.com/Qix-/color-convert)（`MIT`）（包名 `color-convert`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `color-convert` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add color-convert
```

```cp
import {
    Rgb,
    Hsv,
    Hwb,
    Hsl,
    Cmyk,
    ccAbs,
    ccClamp01,
    ccNormHue,
} from "color-convert";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Rgb` | `class` | 字段: `r: u8, g: u8, b: u8` |
| `Hsv` | `class` | 字段: `h: f64, s: f64, v: f64` |
| `Hwb` | `class` | 字段: `h: f64, w: f64, b: f64` |
| `Hsl` | `class` | 字段: `h: f64, s: f64, l: f64` |
| `Cmyk` | `class` | 字段: `c: f64, m: f64, y: f64, k: f64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ccAbs` | `(x: f64): f64` | 调用 `ccAbs` 并返回 `f64`。 |
| `ccClamp01` | `(x: f64): f64` | 调用 `ccClamp01` 并返回 `f64`。 |
| `ccNormHue` | `(h: f64): f64` | 调用 `ccNormHue` 并返回 `f64`。 |
| `ccRoundU8` | `(x: f64): u8` | 调用 `ccRoundU8` 并返回 `u8`。 |
| `ccHexVal` | `(ch: u8): i16` | 调用 `ccHexVal` 并返回 `i16`。 |
| `ccAnsiStd16` | `(idx: u8): Rgb` | 调用 `ccAnsiStd16` 并返回 `Rgb`。 |
| `rgbToHsv` | `(r: u8, g: u8, b: u8): Hsv` | 调用 `rgbToHsv` 并返回 `Hsv`。 |
| `hsvToRgb` | `(h: f64, s: f64, v: f64): Rgb` | 调用 `hsvToRgb` 并返回 `Rgb`。 |
| `rgbToHsl` | `(r: u8, g: u8, b: u8): Hsl` | 调用 `rgbToHsl` 并返回 `Hsl`。 |
| `hslToRgb` | `(h: f64, s: f64, l: f64): Rgb` | 调用 `hslToRgb` 并返回 `Rgb`。 |
| `rgbToHwb` | `(r: u8, g: u8, b: u8): Hwb` | 调用 `rgbToHwb` 并返回 `Hwb`。 |
| `hwbToRgb` | `(h: f64, w: f64, b: f64): Rgb` | 调用 `hwbToRgb` 并返回 `Rgb`。 |
| `rgbToCmyk` | `(r: u8, g: u8, b: u8): Cmyk` | 调用 `rgbToCmyk` 并返回 `Cmyk`。 |
| `cmykToRgb` | `(c: f64, m: f64, y: f64, k: f64): Rgb` | 调用 `cmykToRgb` 并返回 `Rgb`。 |
| `rgbToAnsi256` | `(r: u8, g: u8, b: u8): u8` | 调用 `rgbToAnsi256` 并返回 `u8`。 |
| `ansi256ToRgb` | `(code: u8): Rgb` | 调用 `ansi256ToRgb` 并返回 `Rgb`。 |
| `rgbToAnsi16` | `(r: u8, g: u8, b: u8): u8` | 调用 `rgbToAnsi16` 并返回 `u8`。 |
| `ansi256ToAnsi16` | `(code: u8): u8` | 调用 `ansi256ToAnsi16` 并返回 `u8`。 |
| `ansi16ToRgb` | `(code: u8): Rgb` | 调用 `ansi16ToRgb` 并返回 `Rgb`。 |
| `rgbToHex` | `(r: u8, g: u8, b: u8, out: []u8): usize` | 调用 `rgbToHex` 并返回 `usize`。 |
| `hexToRgb` | `(hex: string): RgbColor` | 调用 `hexToRgb` 并返回 `RgbColor`。 |
| `keywordToRgb` | `(name: string): RgbColor` | 调用 `keywordToRgb` 并返回 `RgbColor`。 |
| `rgbToKeyword` | `(r: u8, g: u8, b: u8): string` | 调用 `rgbToKeyword` 并返回 `string`。 |
| `rgbToGray` | `(r: u8, g: u8, b: u8): f64` | 调用 `rgbToGray` 并返回 `f64`。 |
| `grayToRgb` | `(gray: f64): Rgb` | 调用 `grayToRgb` 并返回 `Rgb`。 |

## 最小调用示例

```cp
import { hexToRgb } from "color-convert";

pub fn main() {
    let result = hexToRgb("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
