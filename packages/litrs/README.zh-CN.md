[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# litrs

Zero-allocation Rust literal lexer, validator, and string unescaper (Rust litrs parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:litrs` ([https://github.com/LukasKalbertodt/litrs](https://github.com/LukasKalbertodt/litrs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `litrs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add litrs
```

```cp
import {
    LiteralKind,
    ParsedLiteral,
    ltrInvalid,
    ltrStrEq,
    ltrIsIdentStart,
    ltrIsIdentCont,
    ltrIsValidSuffix,
    ltrHexVal,
} from "litrs";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `LiteralKind` | `Invalid, Bool, Integer, Float, Char, Byte, String, ByteString` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParsedLiteral` | `class` | 字段: `kind: LiteralKind, raw: string, suffix: string, int_val: u64, bool_val: bool, is_raw_string: bool, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ltrInvalid` | `(src: string): ParsedLiteral` | 调用 `ltrInvalid` 并返回 `ParsedLiteral`。 |
| `ltrStrEq` | `(a: string, b: string): bool` | 调用 `ltrStrEq` 并返回 `bool`。 |
| `ltrIsIdentStart` | `(c: u8): bool` | 调用 `ltrIsIdentStart` 并返回 `bool`。 |
| `ltrIsIdentCont` | `(c: u8): bool` | 调用 `ltrIsIdentCont` 并返回 `bool`。 |
| `ltrIsValidSuffix` | `(s: string): bool` | 调用 `ltrIsValidSuffix` 并返回 `bool`。 |
| `ltrHexVal` | `(c: u8): i32` | 调用 `ltrHexVal` 并返回 `i32`。 |
| `ltrDigitVal` | `(c: u8, radix: u64): i32` | 调用 `ltrDigitVal` 并返回 `i32`。 |
| `ltrParseRawString` | `(src: string, start: usize, kind: LiteralKind): ParsedLiteral` | 调用 `ltrParseRawString` 并返回 `ParsedLiteral`。 |
| `ltrParseQuotedString` | `(src: string, quote_pos: usize, kind: LiteralKind): ParsedLiteral` | 调用 `ltrParseQuotedString` 并返回 `ParsedLiteral`。 |
| `ltrParseCharOrByte` | `(src: string, quote_pos: usize, is_byte: bool): ParsedLiteral` | 调用 `ltrParseCharOrByte` 并返回 `ParsedLiteral`。 |
| `ltrParseNumber` | `(src: string): ParsedLiteral` | 调用 `ltrParseNumber` 并返回 `ParsedLiteral`。 |
| `parseLiteral` | `(src: string): ParsedLiteral` | 通过 `parseLiteral` 解析输入数据，返回 `ParsedLiteral`。 |
| `ltrEncodeUtf8` | `(cp: u32, out: []u8, pos: usize): usize?` | 调用 `ltrEncodeUtf8` 并返回 `usize?`。 |
| `unescapeString` | `(lit_raw: string, out: []u8): i64` | 调用 `unescapeString` 并返回 `i64`。 |

## 最小调用示例

```cp
import { parseLiteral } from "litrs";

pub fn main() {
    let result = parseLiteral("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
