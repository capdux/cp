[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# postcss-value-parser

Zero-allocation CSS dimension, length, angle, time, and resolution parser and unit converter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:postcss-value-parser` ([https://github.com/TrySound/postcss-value-parser](https://github.com/TrySound/postcss-value-parser))（原库名 `postcss-value-parser` → CP 包名 `postcss-value-parser`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `postcss-value-parser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add postcss-value-parser
```

```cp
import {
    CssUnitKind,
    CssDimension,
    CssContext,
    invalidDim,
    parseUnit,
    isValidUnitToken,
    parseCssDimension,
    toPixels,
} from "postcss-value-parser";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `CssUnitKind` | `Px, Cm, Mm, In, Pt, Pc, Q, Rem ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CssDimension` | `class` | 字段: `valid: bool, value: f64, unit: CssUnitKind, unit_str: string` |
| `CssContext` | `class` | 字段: `root_font_px: f64, parent_font_px: f64, viewport_w: f64, viewport_h: f64` · 方法: `defaultCtx` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidDim` | `(): CssDimension` | 调用 `invalidDim` 并返回 `CssDimension`。 |
| `parseUnit` | `(u: string): CssUnitKind` | 通过 `parseUnit` 解析输入数据，返回 `CssUnitKind`。 |
| `isValidUnitToken` | `(u: string): bool` | 执行 `isValidUnitToken` 条件判定，返回 `bool`。 |
| `parseCssDimension` | `(raw: string): CssDimension` | 通过 `parseCssDimension` 解析输入数据，返回 `CssDimension`。 |
| `toPixels` | `(dim: CssDimension, ctx: CssContext): f64?` | 调用 `toPixels` 并返回 `f64?`。 |
| `toDegrees` | `(dim: CssDimension): f64?` | 调用 `toDegrees` 并返回 `f64?`。 |
| `toMilliseconds` | `(dim: CssDimension): f64?` | 调用 `toMilliseconds` 并返回 `f64?`。 |
| `unitCategory` | `(u: CssUnitKind): string` | 调用 `unitCategory` 并返回 `string`。 |

## 最小调用示例

```cp
import { parseUnit } from "postcss-value-parser";

pub fn main() {
    let result = parseUnit("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
