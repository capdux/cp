[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# wcag-contrast

Zero-allocation W3C WCAG 2.1/2.2 color contrast ratio calculator, AA/AAA compliance grader, and accessible foreground color recommender (wcag-contrast / Rust contrast parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:wcag-contrast` ([https://github.com/tmcw/wcag-contrast](https://github.com/tmcw/wcag-contrast))（原库名 `wcag-contrast` → CP 包名 `wcag-contrast`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `wcag-contrast` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add wcag-contrast
```

```cp
import {
    WcagLevel,
    Rgb,
    hexNibble,
    nibbleToHexLower,
    parseHex,
    formatHex,
    linearizeChannel,
    relativeLuminance,
} from "wcag-contrast";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `WcagLevel` | `Fail, AALarge, AA, AAA` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Rgb` | `class` | 字段: `r: u8, g: u8, b: u8` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hexNibble` | `(c: u8): i8` | 调用 `hexNibble` 并返回 `i8`。 |
| `nibbleToHexLower` | `(n: u8): u8` | 调用 `nibbleToHexLower` 并返回 `u8`。 |
| `parseHex` | `(hex: string): Rgb?` | 通过 `parseHex` 解析输入数据，返回 `Rgb?`。 |
| `formatHex` | `(c: Rgb, out: []u8): string` | 通过 `formatHex` 编码或格式化输出，返回 `string`。 |
| `linearizeChannel` | `(channel: u8): f64` | 调用 `linearizeChannel` 并返回 `f64`。 |
| `relativeLuminance` | `(c: Rgb): f64` | 调用 `relativeLuminance` 并返回 `f64`。 |
| `contrastRatio` | `(fg: Rgb, bg: Rgb): f64` | 调用 `contrastRatio` 并返回 `f64`。 |
| `contrastRatioHex` | `(fg_hex: string, bg_hex: string): f64?` | 调用 `contrastRatioHex` 并返回 `f64?`。 |
| `meetsAA` | `(ratio: f64, large_text: bool): bool` | 调用 `meetsAA` 并返回 `bool`。 |
| `meetsAAA` | `(ratio: f64, large_text: bool): bool` | 调用 `meetsAAA` 并返回 `bool`。 |
| `gradeContrast` | `(ratio: f64): WcagLevel` | 调用 `gradeContrast` 并返回 `WcagLevel`。 |
| `bestTextColor` | `(bg: Rgb): Rgb` | 调用 `bestTextColor` 并返回 `Rgb`。 |
| `suggestAccessibleHex` | `(bg_hex: string): string` | 调用 `suggestAccessibleHex` 并返回 `string`。 |

## 最小调用示例

```cp
import { parseHex } from "wcag-contrast";

pub fn main() {
    let result = parseHex("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
