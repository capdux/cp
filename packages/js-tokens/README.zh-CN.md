[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# js-tokens

Zero-allocation tiny JavaScript/TypeScript lexer tokenizer (clean-room MIT implementation of npm js-tokens, #61 / 381M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:js-tokens` ([https://github.com/lydell/js-tokens](https://github.com/lydell/js-tokens)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `js-tokens` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add js-tokens
```

```cp
import {
    JsTokenKind,
    JsToken,
    jstIsIdentStart,
    jstIsIdentPart,
    jstIsDigit,
    jstIsHex,
    jstIsSpace,
    jstStartsWith,
} from "js-tokens";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `JsTokenKind` | `IdentifierName, NumericLiteral, StringLiteral, NoSubstitutionTemplate, TemplateHead, Punctuator, SingleLineComment, MultiLineComment ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `JsToken` | `class` | 字段: `kind: JsTokenKind, start: usize, end: usize, closed: bool` · 方法: `text` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `jstIsIdentStart` | `(b: u8): bool` | 调用 `jstIsIdentStart` 并返回 `bool`。 |
| `jstIsIdentPart` | `(b: u8): bool` | 调用 `jstIsIdentPart` 并返回 `bool`。 |
| `jstIsDigit` | `(b: u8): bool` | 调用 `jstIsDigit` 并返回 `bool`。 |
| `jstIsHex` | `(b: u8): bool` | 调用 `jstIsHex` 并返回 `bool`。 |
| `jstIsSpace` | `(b: u8): bool` | 调用 `jstIsSpace` 并返回 `bool`。 |
| `jstStartsWith` | `(src: string, pos: usize, lit: string): bool` | 调用 `jstStartsWith` 并返回 `bool`。 |
| `nextJsToken` | `(src: string, pos: usize): JsToken?` | 调用 `nextJsToken` 并返回 `JsToken?`。 |
| `tokenizeJs` | `(src: string, out: []JsToken): usize` | 调用 `tokenizeJs` 并返回 `usize`。 |
| `countJsTokens` | `(src: string): usize` | 调用 `countJsTokens` 并返回 `usize`。 |

## 最小调用示例

```cp
import { jstStartsWith } from "js-tokens";

pub fn main() {
    let result = jstStartsWith("example", 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
