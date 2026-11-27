[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ron

Zero-heap Rusty Object Notation (RON) lexer and top-level field extractor in pure CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `ron` |
| 版本 | `0.8.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ron
```

```cp
import {
    RonTokenKind,
    RonToken,
    RonLexer,
    ronpIsWs,
    ronpIsDigit,
    ronpIsHexDigit,
    ronpIsIdentStart,
    ronpIsIdentCont,
} from "ron";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `RonTokenKind` | `Ident, String, Int, Float, Bool, LParen, RParen, LBracket ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RonToken` | `class` | 字段: `kind: RonTokenKind, text: string, start: usize, end: usize` |
| `RonLexer` | `class` | 字段: `src: string, pos: usize` · 方法: `new, nextToken` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ronpIsWs` | `(c: u8): bool` | 调用 `ronpIsWs` 并返回 `bool`。 |
| `ronpIsDigit` | `(c: u8): bool` | 调用 `ronpIsDigit` 并返回 `bool`。 |
| `ronpIsHexDigit` | `(c: u8): bool` | 调用 `ronpIsHexDigit` 并返回 `bool`。 |
| `ronpIsIdentStart` | `(c: u8): bool` | 调用 `ronpIsIdentStart` 并返回 `bool`。 |
| `ronpIsIdentCont` | `(c: u8): bool` | 调用 `ronpIsIdentCont` 并返回 `bool`。 |
| `ronpEq` | `(a: string, b: string): bool` | 调用 `ronpEq` 并返回 `bool`。 |
| `ronpSkipTrivia` | `(src: string, start_pos: usize): usize` | 调用 `ronpSkipTrivia` 并返回 `usize`。 |
| `ronpIsOpenDelim` | `(k: RonTokenKind): bool` | 调用 `ronpIsOpenDelim` 并返回 `bool`。 |
| `ronpIsCloseDelim` | `(k: RonTokenKind): bool` | 调用 `ronpIsCloseDelim` 并返回 `bool`。 |
| `ronGetField` | `(ron_src: string, field_name: string): string?` | 调用 `ronGetField` 并返回 `string?`。 |

## 最小调用示例

```cp
import { ronpEq } from "ron";

pub fn main() {
    let result = ronpEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
