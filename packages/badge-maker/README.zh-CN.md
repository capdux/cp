[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# badge-maker

Zero-heap Shields.io-style SVG status badge generator (flat, flat-square, plastic) with XML escaping and named colors

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:badge-maker` ([https://github.com/badges/shields](https://github.com/badges/shields))（原库名 `badge-maker` → CP 包名 `badge-maker`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `badge-maker` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add badge-maker
```

```cp
import {
    BadgeStyle,
    BadgeOptions,
    BadgeWriter,
    defaultBadge,
    resolveBadgeColor,
    estimateTextWidth,
    renderBadge,
    makeBadge,
} from "badge-maker";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `BadgeStyle` | `Flat, FlatSquare, Plastic` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BadgeOptions` | `class` | 字段: `label: string, message: string, labelColor: string, color: string, style: BadgeStyle` |
| `BadgeWriter` | `class` | 字段: `out: []u8, pos: usize` · 方法: `putStr, writeXmlEscaped, writeUsize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `defaultBadge` | `(label: string, message: string, color: string): BadgeOptions` | 调用 `defaultBadge` 并返回 `BadgeOptions`。 |
| `resolveBadgeColor` | `(name: string): string` | 调用 `resolveBadgeColor` 并返回 `string`。 |
| `estimateTextWidth` | `(s: string): usize` | 调用 `estimateTextWidth` 并返回 `usize`。 |
| `renderBadge` | `(opts: BadgeOptions, out: []u8): string` | 调用 `renderBadge` 并返回 `string`。 |
| `makeBadge` | `(label: string, message: string, color: string, out: []u8): string` | 调用 `makeBadge` 并返回 `string`。 |

## 最小调用示例

```cp
import { defaultBadge } from "badge-maker";

pub fn main() {
    let result = defaultBadge("example", "example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
