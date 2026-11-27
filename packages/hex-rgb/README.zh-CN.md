[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hex-rgb

Zero-allocation CSS hex color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA) parser, formatter, Porter-Duff source-over blender, luminance, and RGB inverter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hex-rgb` ([https://github.com/sindresorhus/hex-rgb](https://github.com/sindresorhus/hex-rgb)) / `npm:colord` ([https://github.com/omgovich/colord](https://github.com/omgovich/colord))（原库名 `hex-rgb` → CP 包名 `hex-rgb`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `hex-rgb` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add hex-rgb
```

```cp
import {
    Rgba,
    parseHexColor,
    formatHex6,
    formatHex8,
    packRgba32,
    unpackRgba32,
    invertRgb,
    isDark,
} from "hex-rgb";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Rgba` | `class` | 字段: `r: u8, g: u8, b: u8, a: u8, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseHexColor` | `(input: string): Rgba` | 通过 `parseHexColor` 解析输入数据，返回 `Rgba`。 |
| `formatHex6` | `(c: Rgba, uppercase: bool, out: []u8): usize` | 通过 `formatHex6` 编码或格式化输出，返回 `usize`。 |
| `formatHex8` | `(c: Rgba, uppercase: bool, out: []u8): usize` | 通过 `formatHex8` 编码或格式化输出，返回 `usize`。 |
| `packRgba32` | `(c: Rgba): u32` | 调用 `packRgba32` 并返回 `u32`。 |
| `unpackRgba32` | `(v: u32): Rgba` | 调用 `unpackRgba32` 并返回 `Rgba`。 |
| `invertRgb` | `(c: Rgba): Rgba` | 调用 `invertRgb` 并返回 `Rgba`。 |
| `isDark` | `(c: Rgba): bool` | 执行 `isDark` 条件判定，返回 `bool`。 |
| `hcClampU8` | `(v: f64): u8` | 调用 `hcClampU8` 并返回 `u8`。 |
| `blendOver` | `(fg: Rgba, bg: Rgba): Rgba` | 调用 `blendOver` 并返回 `Rgba`。 |

## 最小调用示例

```cp
import { parseHexColor } from "hex-rgb";

pub fn main() {
    let result = parseHexColor("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
