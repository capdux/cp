[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# heck

Zero-allocation Rust heck-compatible case conversion engine (PascalCase, camelCase, snake_case, SHOUTY_SNAKE_CASE, kebab-case, SHOUTY-KEBAB-CASE, Title Case, Train-Case) for CP (clean-room MIT).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:heck` ([https://github.com/withoutboats/heck](https://github.com/withoutboats/heck))（原库名 `heck` → CP 包名 `heck`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `heck` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add heck
```

```cp
import {
    HckRune,
    hckDecodeUtf8,
    hckClassify,
    hckToLowerCp,
    hckToUpperCp,
    hckEncodeUtf8,
    hckEmitWord,
    hckConvert,
} from "heck";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HckRune` | `class` | 字段: `cp: u32, len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hckDecodeUtf8` | `(src: string, offset: usize): HckRune` | 调用 `hckDecodeUtf8` 并返回 `HckRune`。 |
| `hckClassify` | `(cp: u32): u8` | 调用 `hckClassify` 并返回 `u8`。 |
| `hckToLowerCp` | `(cp: u32): u32` | 调用 `hckToLowerCp` 并返回 `u32`。 |
| `hckToUpperCp` | `(cp: u32): u32` | 调用 `hckToUpperCp` 并返回 `u32`。 |
| `hckEncodeUtf8` | `(cp: u32, out: []u8, pos: usize): usize` | 调用 `hckEncodeUtf8` 并返回 `usize`。 |
| `hckEmitWord` | `(src: string, start: usize, end: usize, out: []u8, wpos: usize, word_idx: usize, sep: u8, style: u8): usize` | 调用 `hckEmitWord` 并返回 `usize`。 |
| `hckConvert` | `(src: string, out: []u8, sep: u8, style: u8): usize` | 调用 `hckConvert` 并返回 `usize`。 |
| `toUpperCamelCase` | `(src: string, out: []u8): usize` | 调用 `toUpperCamelCase` 并返回 `usize`。 |
| `toLowerCamelCase` | `(src: string, out: []u8): usize` | 调用 `toLowerCamelCase` 并返回 `usize`。 |
| `toSnakeCase` | `(src: string, out: []u8): usize` | 调用 `toSnakeCase` 并返回 `usize`。 |
| `toShoutySnakeCase` | `(src: string, out: []u8): usize` | 调用 `toShoutySnakeCase` 并返回 `usize`。 |
| `toKebabCase` | `(src: string, out: []u8): usize` | 调用 `toKebabCase` 并返回 `usize`。 |
| `toShoutyKebabCase` | `(src: string, out: []u8): usize` | 调用 `toShoutyKebabCase` 并返回 `usize`。 |
| `toTitleCase` | `(src: string, out: []u8): usize` | 调用 `toTitleCase` 并返回 `usize`。 |
| `toTrainCase` | `(src: string, out: []u8): usize` | 调用 `toTrainCase` 并返回 `usize`。 |

## 最小调用示例

```cp
import { hckDecodeUtf8 } from "heck";

pub fn main() {
    let result = hckDecodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
