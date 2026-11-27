[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bcp-47

Zero-allocation RFC 5646 / BCP-47 language tag parser, canonicalizer, and fallback locale matcher for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bcp-47` ([https://github.com/words/bcp-47](https://github.com/words/bcp-47))（原库名 `bcp-47` → CP 包名 `bcp-47`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `bcp-47` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bcp-47
```

```cp
import {
    Bcp47Tag,
    SubtagSpan,
    eqSubtagNormalized,
    isAllAlpha,
    isAllDigit,
    isAllAlphaNum,
    isVariantSubtag,
    isSingletonSubtag,
} from "bcp-47";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Bcp47Tag` | `class` | 字段: `valid: bool, language: string, script: string, region: string, variant: string, extension: string, +1 项` · 方法: `invalid` |
| `SubtagSpan` | `class` | 字段: `start: usize, end: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `eqSubtagNormalized` | `(a: string, b: string): bool` | 调用 `eqSubtagNormalized` 并返回 `bool`。 |
| `isAllAlpha` | `(s: string): bool` | 执行 `isAllAlpha` 条件判定，返回 `bool`。 |
| `isAllDigit` | `(s: string): bool` | 执行 `isAllDigit` 条件判定，返回 `bool`。 |
| `isAllAlphaNum` | `(s: string): bool` | 执行 `isAllAlphaNum` 条件判定，返回 `bool`。 |
| `isVariantSubtag` | `(s: string): bool` | 执行 `isVariantSubtag` 条件判定，返回 `bool`。 |
| `isSingletonSubtag` | `(s: string): bool` | 执行 `isSingletonSubtag` 条件判定，返回 `bool`。 |
| `isPrivateIntro` | `(s: string): bool` | 执行 `isPrivateIntro` 条件判定，返回 `bool`。 |
| `parseBcp47` | `(raw: string): Bcp47Tag` | 通过 `parseBcp47` 解析输入数据，返回 `Bcp47Tag`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `appendLowerHyphen` | `(out: []u8, start_pos: usize, s: string): usize` | 调用 `appendLowerHyphen` 并返回 `usize`。 |
| `appendUpper` | `(out: []u8, start_pos: usize, s: string): usize` | 调用 `appendUpper` 并返回 `usize`。 |
| `appendTitle` | `(out: []u8, start_pos: usize, s: string): usize` | 调用 `appendTitle` 并返回 `usize`。 |
| `canonicalizeBcp47` | `(tag: Bcp47Tag, out: []u8): string` | 执行 `canonicalizeBcp47` 条件判定，返回 `bool`。 |
| `scoreLocaleMatch` | `(req: Bcp47Tag, cand: Bcp47Tag): u32` | 调用 `scoreLocaleMatch` 并返回 `u32`。 |
| `matchLocale` | `(requested: Bcp47Tag, supported: &[]string): string?` | 调用 `matchLocale` 并返回 `string?`。 |

## 最小调用示例

```cp
import { isAllAlpha } from "bcp-47";

pub fn main() {
    let result = isAllAlpha("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
