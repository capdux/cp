[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# language-tags

Zero-heap BCP 47 / RFC 5646 language tag parser, canonical formatter, and RFC 4647 range matcher in pure CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `language-tags` |
| 版本 | `1.0.9` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add language-tags
```

```cp
import {
    Bcp47Tag,
    ltgIsAlpha,
    ltgIsDigit,
    ltgIsAlnum,
    ltgIsSep,
    ltgToLower,
    ltgToUpper,
    ltgAllAlpha,
} from "language-tags";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Bcp47Tag` | `class` | 字段: `language: string, script: string, region: string, variant: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ltgIsAlpha` | `(c: u8): bool` | 调用 `ltgIsAlpha` 并返回 `bool`。 |
| `ltgIsDigit` | `(c: u8): bool` | 调用 `ltgIsDigit` 并返回 `bool`。 |
| `ltgIsAlnum` | `(c: u8): bool` | 调用 `ltgIsAlnum` 并返回 `bool`。 |
| `ltgIsSep` | `(c: u8): bool` | 调用 `ltgIsSep` 并返回 `bool`。 |
| `ltgToLower` | `(c: u8): u8` | 调用 `ltgToLower` 并返回 `u8`。 |
| `ltgToUpper` | `(c: u8): u8` | 调用 `ltgToUpper` 并返回 `u8`。 |
| `ltgAllAlpha` | `(s: string): bool` | 调用 `ltgAllAlpha` 并返回 `bool`。 |
| `ltgAllDigit` | `(s: string): bool` | 调用 `ltgAllDigit` 并返回 `bool`。 |
| `ltgAllAlnum` | `(s: string): bool` | 调用 `ltgAllAlnum` 并返回 `bool`。 |
| `ltgInvalidTag` | `(): Bcp47Tag` | 调用 `ltgInvalidTag` 并返回 `Bcp47Tag`。 |
| `parseLanguageTag` | `(tag: string): Bcp47Tag` | 通过 `parseLanguageTag` 解析输入数据，返回 `Bcp47Tag`。 |
| `ltgPushByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `ltgPushByte` 并返回 `usize`。 |
| `formatCanonicalTag` | `(t: Bcp47Tag, out: []u8): usize` | 通过 `formatCanonicalTag` 编码或格式化输出，返回 `usize`。 |
| `matchesLanguageRange` | `(tag: string, range: string): bool` | 调用 `matchesLanguageRange` 并返回 `bool`。 |

## 最小调用示例

```cp
import { parseLanguageTag } from "language-tags";

pub fn main() {
    let result = parseLanguageTag("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
