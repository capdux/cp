[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# boxen

Zero-allocation CLI terminal box renderer with 6 Unicode/ASCII border styles, embedded titles, alignment, CJK double-width & ANSI escape awareness (boxen parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:boxen` ([https://github.com/sindresorhus/boxen](https://github.com/sindresorhus/boxen))

| 元信息 | 值 |
| --- | --- |
| 包名 | `boxen` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add boxen
```

```cp
import {
    BorderStyle,
    TextAlign,
    BoxOptions,
    BorderChars,
    Utf8Step,
    borderForStyle,
    decodeUtf8At,
    codepointWidth,
} from "boxen";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `BorderStyle` | `Single, Double, Round, Bold, Classic, SingleDouble` |
| `enum` | `TextAlign` | `Left, Center, Right` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BoxOptions` | `class` | 字段: `style: BorderStyle, text_align: TextAlign, padding_x: u8, padding_y: u8, margin_left: u8, title: string, +1 项` · 方法: `defaults` |
| `BorderChars` | `class` | 字段: `tl: string, top: string, tr: string, left: string, right: string, bl: string, +2 项` |
| `Utf8Step` | `class` | 字段: `cp: u32, width: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `borderForStyle` | `(style: BorderStyle): BorderChars` | 调用 `borderForStyle` 并返回 `BorderChars`。 |
| `decodeUtf8At` | `(s: string, i: usize): Utf8Step` | 通过 `decodeUtf8At` 解析输入数据，返回 `Utf8Step`。 |
| `codepointWidth` | `(cp: u32): usize` | 调用 `codepointWidth` 并返回 `usize`。 |
| `displayWidth` | `(s: string): usize` | 调用 `displayWidth` 并返回 `usize`。 |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | 写入或更新状态数据。 |
| `pushStr` | `(out: []u8, pos: usize, s: string): usize` | 写入或更新状态数据。 |
| `pushRepeat` | `(out: []u8, pos: usize, s: string, count: usize): usize` | 写入或更新状态数据。 |
| `pushSpaces` | `(out: []u8, pos: usize, count: usize): usize` | 写入或更新状态数据。 |
| `renderBox` | `(text: string, opts: BoxOptions, out: []u8): string` | 调用 `renderBox` 并返回 `string`。 |
| `box` | `(text: string, out: []u8): string` | 调用 `box` 并返回 `string`。 |
| `titledBox` | `(title: string, text: string, style: BorderStyle, out: []u8): string` | 调用 `titledBox` 并返回 `string`。 |

## 最小调用示例

```cp
import { decodeUtf8At } from "boxen";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
