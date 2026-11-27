[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unicode-segmentation

Zero-allocation UAX #29 Unicode extended grapheme cluster and word boundary segmentation for CP (clean-room MIT).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-segmentation` ([https://github.com/unicode-rs/unicode-segmentation](https://github.com/unicode-rs/unicode-segmentation))

| 元信息 | 值 |
| --- | --- |
| 包名 | `unicode-segmentation` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unicode-segmentation
```

```cp
import {
    GraphemeSpan,
    UsegRune,
    GraphemeIter,
    WordIter,
    usegDecodeUtf8,
    usegIsExtend,
    usegIsPrepend,
    usegIsSpacingMark,
} from "unicode-segmentation";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GraphemeSpan` | `class` | 字段: `start: usize, end: usize` · 方法: `byteLen` |
| `UsegRune` | `class` | 字段: `cp: u32, len: usize` |
| `GraphemeIter` | `class` | 字段: `src: string, pos: usize` · 方法: `new, next, reset` |
| `WordIter` | `class` | 字段: `src: string, pos: usize` · 方法: `new, reset, next, nextUnicodeWord` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `usegDecodeUtf8` | `(src: string, offset: usize): UsegRune` | 调用 `usegDecodeUtf8` 并返回 `UsegRune`。 |
| `usegIsExtend` | `(cp: u32): bool` | 调用 `usegIsExtend` 并返回 `bool`。 |
| `usegIsPrepend` | `(cp: u32): bool` | 调用 `usegIsPrepend` 并返回 `bool`。 |
| `usegIsSpacingMark` | `(cp: u32): bool` | 调用 `usegIsSpacingMark` 并返回 `bool`。 |
| `usegIsControl` | `(cp: u32): bool` | 调用 `usegIsControl` 并返回 `bool`。 |
| `usegIsExtPic` | `(cp: u32): bool` | 调用 `usegIsExtPic` 并返回 `bool`。 |
| `usegGraphemeProp` | `(cp: u32): u8` | 调用 `usegGraphemeProp` 并返回 `u8`。 |
| `usegNextGraphemeEnd` | `(src: string, start: usize): usize` | 调用 `usegNextGraphemeEnd` 并返回 `usize`。 |
| `countGraphemes` | `(src: string): usize` | 调用 `countGraphemes` 并返回 `usize`。 |
| `nthGrapheme` | `(src: string, idx: usize): GraphemeSpan?` | 调用 `nthGrapheme` 并返回 `GraphemeSpan?`。 |
| `sliceGraphemes` | `(src: string, start_idx: usize, max_count: usize): GraphemeSpan` | 调用 `sliceGraphemes` 并返回 `GraphemeSpan`。 |
| `usegIsCjkWordChar` | `(cp: u32): bool` | 调用 `usegIsCjkWordChar` 并返回 `bool`。 |
| `usegIsALetter` | `(cp: u32): bool` | 调用 `usegIsALetter` 并返回 `bool`。 |
| `usegIsNumeric` | `(cp: u32): bool` | 调用 `usegIsNumeric` 并返回 `bool`。 |
| `usegWordProp` | `(cp: u32): u8` | 调用 `usegWordProp` 并返回 `u8`。 |
| `usegPeekWordProp` | `(src: string, offset: usize): u8` | 调用 `usegPeekWordProp` 并返回 `u8`。 |
| `usegIsWordSpan` | `(src: string, start: usize, end: usize): bool` | 调用 `usegIsWordSpan` 并返回 `bool`。 |
| `countUnicodeWords` | `(src: string): usize` | 调用 `countUnicodeWords` 并返回 `usize`。 |

## 最小调用示例

```cp
import { usegDecodeUtf8 } from "unicode-segmentation";

pub fn main() {
    let result = usegDecodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
