[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# wcag-contrast

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:wcag-contrast` ([https://github.com/tmcw/wcag-contrast](https://github.com/tmcw/wcag-contrast))（原库名 `wcag-contrast` → CP 包名 `wcag-contrast`）

零堆分配的 W3C WCAG 2.1/2.2 色彩无障碍对比度计算、AA/AAA 评级与自动高对比前景色推荐引擎 — 洁净室 MIT 实现，对标 npm `wcag-contrast` / `color-contrast-checker` 与 Rust `contrast`。

Zero-allocation W3C WCAG 2.1/2.2 color contrast calculator, AA/AAA compliance grader, and accessible foreground color recommender for CP — clean-room MIT parity with npm `wcag-contrast` and Rust `contrast`.

## 特性 / Features

- **精确 W3C WCAG 2.1/2.2 相对亮度公式 / Exact W3C sRGB luminance**：`0.2126 * R + 0.7152 * G + 0.0722 * B`，完整非线性伽马解码 `((c + 0.055) / 1.055)^2.4`。
- **完整 HEX 解析与格式化 / Flexible HEX support**：支持 `#RGB`、`RGB`、`#RRGGBB`、`RRGGBB`（大小写无关）及零分配 `formatHex`。
- **AA / AAA 合规判定与前景色推荐 / Compliance & auto-recommendation**：支持普通文本与大字号阈值判定，自动为任意深色/浅色背景推荐最高对比度的黑/白前景色。

## 安装 / Install

```bash
cap add wcag-contrast
```

## API 参考 / API Reference

| 符号 / Symbol | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `Rgb` | `{ r: u8, g: u8, b: u8 }` | 8-bit sRGB 颜色结构体 |
| `WcagLevel` | `enum: u8 { Fail, AALarge, AA, AAA }` | WCAG 对比度等级枚举 |
| `parseHex` | `(hex: string) -> Rgb?` | 解析 `#RGB`/`RGB`/`#RRGGBB`/`RRGGBB`（大小写无关） |
| `formatHex` | `(c: Rgb, out: []u8) -> string` | 零分配格式化为 `#rrggbb` |
| `relativeLuminance` | `(c: Rgb) -> f64` | 计算 W3C 相对亮度 `[0.0, 1.0]` |
| `contrastRatio` | `(fg: Rgb, bg: Rgb) -> f64` | 计算对比度 `[1.0, 21.0]` |
| `contrastRatioHex` | `(fg_hex: string, bg_hex: string) -> f64?` | 直接计算两个 HEX 颜色的对比度 |
| `meetsAA` | `(ratio: f64, large_text: bool) -> bool` | AA 达标判定（普通 `>= 4.5`，大字 `>= 3.0`） |
| `meetsAAA` | `(ratio: f64, large_text: bool) -> bool` | AAA 达标判定（普通 `>= 7.0`，大字 `>= 4.5`） |
| `gradeContrast` | `(ratio: f64) -> WcagLevel` | 返回 `Fail` / `AALarge` / `AA` / `AAA` |
| `bestTextColor` | `(bg: Rgb) -> Rgb` | 在黑/白中返回对 `bg` 对比度更高者 |
| `suggestAccessibleHex` | `(bg_hex: string) -> string` | 返回 `"#000000"` 或 `"#ffffff"` |

## 使用示例 / Usage

```cp
import {
    Rgb,
    WcagLevel,
    parseHex,
    formatHex,
    contrastRatioHex,
    meetsAA,
    gradeContrast,
    suggestAccessibleHex,
} from "wcag-contrast";

const ratio = contrastRatioHex("#111827", "#ffffff").?; // ~17.74
const ok_aa = meetsAA(ratio, false);                    // true
const level = gradeContrast(ratio);                     // WcagLevel.AAA

const fg_on_navy = suggestAccessibleHex("#0f172a");     // "#ffffff"
const fg_on_yellow = suggestAccessibleHex("#fde047");   // "#000000"

let buf: [8]u8 = undefined;
const hex = formatHex(Rgb{ r: 15, g: 23, b: 42 }, buf[0..8]); // "#0f172a"
```

## License

MIT
