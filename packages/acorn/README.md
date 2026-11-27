# @cap/acorn-parse

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:acorn` ([https://github.com/acornjs/acorn](https://github.com/acornjs/acorn))（原库名 `acorn` → CP 包名 `@cap/acorn-parse`）

Zero-allocation ES2024 JavaScript lexer and ES Module import/export scanner for CP — clean-room MIT implementation inspired by npm `acorn` v8.

纯 CP 零堆分配 ES2024 JavaScript 词法分析器与模块导入/导出扫描器（对标 npm `acorn` v8），支持精确 1-based 行列号追踪、括号平衡校验与零拷贝 ESM `import` 说明符提取。

## Install / 安装

```bash
cap add @cap/acorn-parse
```

## API

### Token Kind Constants / 令牌类型常量

| Constant | Value | Description |
| --- | --- | --- |
| `TOK_EOF` | `0` | End of input |
| `TOK_IDENT` | `1` | Identifier (`foo`, `$bar`, `_baz`) |
| `TOK_KEYWORD` | `2` | ES keyword (`import`, `export`, `from`, `function`, `const`, `let`, `var`, `class`, `async`, `await`, `return`, `if`, `else`, `for`, `while`, `try`, `catch`, `throw`, `new`, etc.) |
| `TOK_NUMBER` | `3` | Numeric literal (decimal, `0x` hex, `0b` binary, `0o` octal, float, scientific `1e-5`, BigInt `42n`) |
| `TOK_STRING` | `4` | Single/double quoted string with escape handling |
| `TOK_TEMPLATE` | `5` | Template literal `` `hello ${name}` `` |
| `TOK_PUNCT` | `6` | Single & multi-char operators (`===`, `!==`, `=>`, `??`, `?.`, `...`, `&&`, `||`, etc.) |
| `TOK_COMMENT` | `7` | Line (`//`) and block (`/* ... */`) comments |

### Structs & Functions / 核心类型与函数

| Symbol | Signature | Description |
| --- | --- | --- |
| `JsToken` | `struct { kind: u8, start: usize, end: usize, line: usize, col: usize }` | Lexical token with byte span & 1-based line/col |
| `JsLexer` | `JsLexer.new(src)` + `nextToken(*_): JsToken` | Zero-alloc pull-based ES2024 tokenizer |
| `JsModuleSummary` | `struct { ok: bool, token_count: usize, import_count: usize, export_count: usize, function_count: usize, class_count: usize, unmatched_bracket_pos: usize }` | Module syntax & bracket balance summary |
| `analyzeJsModule` | `(src: string): JsModuleSummary` | Validate `()`/`[]`/`{}` balance and count module declarations |
| `extractImportSpecifiers` | `(src: string, out_specs: []string): usize` | Zero-copy extraction of static ESM `import ... from "spec"` and `import "spec"` specifiers |

## Usage / 使用示例

```cp
import {
    JsLexer,
    TOK_EOF,
    analyzeJsModule,
    extractImportSpecifiers,
} from "@cap/acorn-parse";

const src = "import { a } from './mod.js';\nexport function run() { return a ?? 1; }";
const summary = analyzeJsModule(src); // ok == true, import_count == 1, export_count == 1, function_count == 1

let specs: [8]string = undefined;
const n = extractImportSpecifiers(src, specs[0..8]); // specs[0] == "./mod.js"
```
