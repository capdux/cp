[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# parse-json

Zero-allocation RFC 8259 JSON diagnostic validator with line/column locations and codeFrame rendering (npm parse-json v8 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse-json` ([https://github.com/sindresorhus/parse-json](https://github.com/sindresorhus/parse-json))（原库名 `parse-json` → CP 包名 `parse-json`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `parse-json` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add parse-json
```

```cp
import {
    JsonDiagKind,
    JsonDiagResult,
    PjsParser,
    PjsLoc,
    PjsLineSpan,
    PjsWriter,
    pjsIsWs,
    pjsIsDigit,
} from "parse-json";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `JsonDiagKind` | `Ok, EmptyInput, UnexpectedToken, UnexpectedEnd, TrailingComma, SingleQuotedString, UnquotedKey, LeadingZero ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `JsonDiagResult` | `class` | 字段: `ok: bool, kind: JsonDiagKind, offset: usize, line: usize, column: usize, value_kind: u8` |
| `PjsParser` | `class` | 字段: `input: string, pos: usize, has_err: bool, err_kind: JsonDiagKind, err_offset: usize` |
| `PjsLoc` | `class` | 字段: `line: usize, column: usize` |
| `PjsLineSpan` | `class` | 字段: `found: bool, start: usize, end: usize` |
| `PjsWriter` | `class` | 字段: `buf: []u8, pos: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pjsIsWs` | `(ch: u8): bool` | 调用 `pjsIsWs` 并返回 `bool`。 |
| `pjsIsDigit` | `(ch: u8): bool` | 调用 `pjsIsDigit` 并返回 `bool`。 |
| `pjsIsHex` | `(ch: u8): bool` | 调用 `pjsIsHex` 并返回 `bool`。 |
| `pjsIsIdentStart` | `(ch: u8): bool` | 调用 `pjsIsIdentStart` 并返回 `bool`。 |
| `pjsSkipWs` | `(p: &mut PjsParser)` | 就地执行 `pjsSkipWs` 操作。 |
| `pjsSetErr` | `(p: &mut PjsParser, kind: JsonDiagKind, offset: usize)` | 就地执行 `pjsSetErr` 操作。 |
| `pjsLineCol` | `(input: string, offset: usize): PjsLoc` | 调用 `pjsLineCol` 并返回 `PjsLoc`。 |
| `pjsParseString` | `(p: &mut PjsParser)` | 就地执行 `pjsParseString` 操作。 |
| `pjsParseNumber` | `(p: &mut PjsParser)` | 就地执行 `pjsParseNumber` 操作。 |
| `pjsExpectKeyword` | `(p: &mut PjsParser, kw: string)` | 就地执行 `pjsExpectKeyword` 操作。 |
| `pjsParseValue` | `(p: &mut PjsParser, depth: usize): u8` | 调用 `pjsParseValue` 并返回 `u8`。 |
| `validateJsonWithDiag` | `(input: string): JsonDiagResult` | 校验输入合法性，返回 `JsonDiagResult`。 |
| `pjsDiagMessage` | `(kind: JsonDiagKind): string` | 调用 `pjsDiagMessage` 并返回 `string`。 |
| `pjsWriteByte` | `(w: &mut PjsWriter, b: u8)` | 就地执行 `pjsWriteByte` 操作。 |
| `pjsWriteStr` | `(w: &mut PjsWriter, s: string)` | 就地执行 `pjsWriteStr` 操作。 |
| `pjsNumWidth` | `(n: usize): usize` | 调用 `pjsNumWidth` 并返回 `usize`。 |
| `pjsWriteUsize` | `(w: &mut PjsWriter, n: usize)` | 就地执行 `pjsWriteUsize` 操作。 |
| `pjsWriteSpaces` | `(w: &mut PjsWriter, count: usize)` | 就地执行 `pjsWriteSpaces` 操作。 |
| `pjsFindLineSpan` | `(input: string, target_line: usize): PjsLineSpan` | 调用 `pjsFindLineSpan` 并返回 `PjsLineSpan`。 |
| `pjsWriteFrameLine` | `(w: &mut PjsWriter, input: string, line_no: usize, span: PjsLineSpan, gutter_w: usize, is_err: bool)` | 就地执行 `pjsWriteFrameLine` 操作。 |
| `formatJsonError` | `(input: string, diag: JsonDiagResult, filename: string, out: []u8): usize` | 通过 `formatJsonError` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { validateJsonWithDiag } from "parse-json";

pub fn main() {
    let result = validateJsonWithDiag("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
