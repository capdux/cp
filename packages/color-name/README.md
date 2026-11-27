[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# color-name

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:color-name` ([https://github.com/colorjs/color-name](https://github.com/colorjs/color-name))（原库名 `color-name` → CP 包名 `color-name`）

Zero-allocation 148 W3C CSS Color 4 named colors lookup and exact RGB ↔ HSL ↔ HSV ↔ HEX color space conversion engine for CP — clean-room MIT parity with npm `color-name` and `color-convert`.

零堆分配的 CP 颜色名称与色空间转换引擎，完整内置 148 个 W3C CSS Color 4 标准命名颜色（`O(log N)` 二分查找 + 反向查名），提供精确 RGB ↔ HSL ↔ HSV ↔ HEX 互转与 `mixRgb`/`lighten`/`darken` 调色工具（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add color-name
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Rgb` / `Hsl` / `Hsv` | `pub struct` | RGB (`0..255`), HSL (`h: 0..360, s/l: 0..1`), HSV (`h: 0..360, s/v: 0..1`) |
| `cssColorCount` | `() -> usize` | 返回内置 W3C CSS Color 4 颜色总数（`148`） |
| `rgbFromCssName` | `(name: string) -> Rgb?` | 按 CSS 命名查 RGB（大小写无关、忽略首尾空白、二分查找） |
| `cssNameFromRgb` | `(c: Rgb) -> string?` | 按 RGB 反查 W3C 标准颜色名 |
| `rgbToHsl` / `hslToRgb` | `(Rgb) -> Hsl` / `(Hsl) -> Rgb` | RGB ↔ HSL 精确色空间互转 |
| `rgbToHsv` / `hsvToRgb` | `(Rgb) -> Hsv` / `(Hsv) -> Rgb` | RGB ↔ HSV 精确色空间互转 |
| `rgbToHex` / `hexToRgb` | `(Rgb, []u8) -> string` / `(string) -> Rgb?` | RGB ↔ `#rrggbb` / `#rgb` 编解码 |
| `mixRgb` | `(a: Rgb, b: Rgb, weight: f64) -> Rgb` | 线性混色（`weight` in `0.0..=1.0`） |
| `lighten` / `darken` | `(c: Rgb, amount: f64) -> Rgb` | 基于 HSL 亮度增亮 / 压暗 |

## Usage / 使用示例

```cp
import {
    Rgb,
    rgbFromCssName,
    cssNameFromRgb,
    rgbToHsl,
    hslToRgb,
    rgbToHex,
    hexToRgb,
    mixRgb,
    lighten,
    darken,
} from "color-name";

let hex_buf: [16]u8 = undefined;
const purple = rgbFromCssName("RebeccaPurple").?; // Rgb{ r: 102, g: 51, b: 153 }
const hex = rgbToHex(purple, hex_buf[0..16]);     // "#663399"
const name = cssNameFromRgb(Rgb.from(255, 99, 71)); // "tomato"
const lighter = lighten(purple, 0.15);
```

## License

MIT
