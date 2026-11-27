[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# nom

Zero-allocation streaming parser combinator primitives (tag, tagNoCase, take, takeUntil, character classes, delimited/separated sequences, integer parsers) for CP (clean-room MIT, inspired by Rust nom v7).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:nom` ([https://github.com/rust-bakery/nom](https://github.com/rust-bakery/nom))（原库名 `nom` → CP 包名 `nom`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `nom` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add nom
```

```cp
import {
    ParseRes,
    IntRes,
    UintRes,
    nomFail,
    nomOk,
    nomContainsByte,
    nomIsDigit,
    nomIsHexDigit,
} from "nom";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParseRes` | `class` | 字段: `ok: bool, rest: string, matched: string` |
| `IntRes` | `class` | 字段: `ok: bool, rest: string, value: i64` |
| `UintRes` | `class` | 字段: `ok: bool, rest: string, value: u64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `nomFail` | `(input: string): ParseRes` | 调用 `nomFail` 并返回 `ParseRes`。 |
| `nomOk` | `(input: string, split_at: usize): ParseRes` | 调用 `nomOk` 并返回 `ParseRes`。 |
| `nomContainsByte` | `(set: string, b: u8): bool` | 调用 `nomContainsByte` 并返回 `bool`。 |
| `nomIsDigit` | `(b: u8): bool` | 调用 `nomIsDigit` 并返回 `bool`。 |
| `nomIsHexDigit` | `(b: u8): bool` | 调用 `nomIsHexDigit` 并返回 `bool`。 |
| `nomHexVal` | `(b: u8): u64` | 调用 `nomHexVal` 并返回 `u64`。 |
| `nomIsAlpha` | `(b: u8): bool` | 调用 `nomIsAlpha` 并返回 `bool`。 |
| `nomIsAlnum` | `(b: u8): bool` | 调用 `nomIsAlnum` 并返回 `bool`。 |
| `nomIsSpace` | `(b: u8): bool` | 调用 `nomIsSpace` 并返回 `bool`。 |
| `nomIsMultispace` | `(b: u8): bool` | 调用 `nomIsMultispace` 并返回 `bool`。 |
| `nomSliceEq` | `(a: string, b: string): bool` | 调用 `nomSliceEq` 并返回 `bool`。 |
| `tag` | `(input: string, expected: string): ParseRes` | 调用 `tag` 并返回 `ParseRes`。 |
| `tagNoCase` | `(input: string, expected: string): ParseRes` | 调用 `tagNoCase` 并返回 `ParseRes`。 |
| `take` | `(input: string, count: usize): ParseRes` | 调用 `take` 并返回 `ParseRes`。 |
| `takeUntil` | `(input: string, needle: string): ParseRes` | 调用 `takeUntil` 并返回 `ParseRes`。 |
| `isNot` | `(input: string, forbidden_chars: string): ParseRes` | 执行 `isNot` 条件判定，返回 `bool`。 |
| `isA` | `(input: string, allowed_chars: string): ParseRes` | 执行 `isA` 条件判定，返回 `bool`。 |
| `digit0` | `(input: string): ParseRes` | 调用 `digit0` 并返回 `ParseRes`。 |
| `digit1` | `(input: string): ParseRes` | 调用 `digit1` 并返回 `ParseRes`。 |
| `hexDigit1` | `(input: string): ParseRes` | 调用 `hexDigit1` 并返回 `ParseRes`。 |
| `alpha0` | `(input: string): ParseRes` | 调用 `alpha0` 并返回 `ParseRes`。 |
| `alpha1` | `(input: string): ParseRes` | 调用 `alpha1` 并返回 `ParseRes`。 |
| `alphanumeric0` | `(input: string): ParseRes` | 调用 `alphanumeric0` 并返回 `ParseRes`。 |
| `alphanumeric1` | `(input: string): ParseRes` | 调用 `alphanumeric1` 并返回 `ParseRes`。 |
| `space0` | `(input: string): ParseRes` | 调用 `space0` 并返回 `ParseRes`。 |
| `space1` | `(input: string): ParseRes` | 调用 `space1` 并返回 `ParseRes`。 |
| `multispace0` | `(input: string): ParseRes` | 调用 `multispace0` 并返回 `ParseRes`。 |
| `multispace1` | `(input: string): ParseRes` | 调用 `multispace1` 并返回 `ParseRes`。 |
| `lineEnding` | `(input: string): ParseRes` | 调用 `lineEnding` 并返回 `ParseRes`。 |
| `notLineEnding` | `(input: string): ParseRes` | 调用 `notLineEnding` 并返回 `ParseRes`。 |
| `delimitedTag` | `(input: string, open: string, close: string): ParseRes` | 调用 `delimitedTag` 并返回 `ParseRes`。 |
| `precededTag` | `(input: string, prefix: string): ParseRes` | 调用 `precededTag` 并返回 `ParseRes`。 |
| `terminatedTag` | `(input: string, suffix: string): ParseRes` | 调用 `terminatedTag` 并返回 `ParseRes`。 |
| `separatedPairTag` | `(input: string, sep: string): ParseRes` | 调用 `separatedPairTag` 并返回 `ParseRes`。 |
| `parseU64` | `(input: string): UintRes` | 通过 `parseU64` 解析输入数据，返回 `UintRes`。 |
| `parseI64` | `(input: string): IntRes` | 通过 `parseI64` 解析输入数据，返回 `IntRes`。 |
| `parseHexU64` | `(input: string): UintRes` | 通过 `parseHexU64` 解析输入数据，返回 `UintRes`。 |

## 最小调用示例

```cp
import { isNot } from "nom";

pub fn main() {
    let result = isNot("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
