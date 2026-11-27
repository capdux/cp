[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/parse-json

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse-json` ([https://github.com/sindresorhus/parse-json](https://github.com/sindresorhus/parse-json))（原库名 `parse-json` → CP 包名 `@cap/parse-json`）

Zero-allocation RFC 8259 JSON diagnostic validator with precise line/column reporting and code frame rendering for CP — clean-room MIT implementation inspired by Sindre Sorhus `parse-json` v8.

纯 CP 零堆分配 JSON 诊断验证器（对标 `parse-json` v8），严格执行 RFC 8259 递归下降校验，精准定位首个语法错误的字节偏移与 1-based 行列号，专项识别尾逗号、单引号字符串、未加引号键名、前导零与未转义控制字符，并渲染带 `^` 指示符的 `codeFrame`。

## Install / 安装

```bash
cap add @cap/parse-json
```

## API

### Enums, Constants & Structs / 核心类型与常量

| Symbol | Description |
| --- | --- |
| `JsonDiagKind` | `Ok`, `EmptyInput`, `UnexpectedToken`, `UnexpectedEnd`, `TrailingComma`, `SingleQuotedString`, `UnquotedKey`, `LeadingZero`, `UnescapedControlChar`, `MaxDepthExceeded` |
| `JsonDiagResult` | `struct { ok: bool, kind: JsonDiagKind, offset: usize, line: usize, column: usize, value_kind: u8 }` |
| `JSON_VAL_*` | Value kind tags (`NONE=0`, `OBJECT=1`, `ARRAY=2`, `STRING=3`, `NUMBER=4`, `BOOL=5`, `NULL=6`) |

### Functions / 核心函数

| Symbol | Signature | Description |
| --- | --- | --- |
| `validateJsonWithDiag` | `(input: string): JsonDiagResult` | Validate RFC 8259 JSON and report precise diagnostic kind, offset, 1-based line and column |
| `formatJsonError` | `(input: string, diag: JsonDiagResult, filename: string, out: []u8): usize` | Render diagnostic message and `codeFrame` with line numbers and `^` caret into `out` |

## Usage / 使用示例

```cp
import { validateJsonWithDiag, formatJsonError, JsonDiagKind } from "@cap/parse-json";

const bad = "{\n  \"a\": 1,\n}";
const diag = validateJsonWithDiag(bad);
// diag.ok == false, diag.kind == JsonDiagKind.TrailingComma, diag.line == 2, diag.column == 9

let buf: [512]u8 = undefined;
const n = formatJsonError(bad, diag, "config.json", buf[0..512]);
```
