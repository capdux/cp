# @cap/syntax-highlight

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:highlight.js` ([https://github.com/highlightjs/highlight.js](https://github.com/highlightjs/highlight.js))（原库名 `highlight.js` → CP 包名 `@cap/syntax-highlight`）

Zero-allocation multi-language lexer and syntax highlighter (Code, JSON, SQL) with ANSI terminal coloring and XSS-safe HTML output for CP — clean-room MIT parity with npm `highlight.js` (58M/wk) and `prismjs` (50M/wk).

纯 CP 零堆分配多语言词法分析与语法高亮引擎，支持 CP/TS/Rust/C 通用代码、JSON 与 SQL，提供零拷贝 `Token` 切片、ANSI 终端彩色输出与防 XSS 转义 HTML `<span class="hl-...">` 输出。

## Install / 安装

```bash
cap add @cap/syntax-highlight
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `TOK_*` | `TOK_PLAIN(0)` .. `TOK_PUNCT(7)` | Token kind constants |
| `LANG_*` | `LANG_CODE(0)`, `LANG_JSON(1)`, `LANG_SQL(2)` | Built-in language modes |
| `Token` | `struct { kind: u8, start: usize, len: usize }` | Zero-copy source token span |
| `tokenize` | `(src: []const u8, lang: u8, out: []Token) -> usize` | Split source into contiguous tokens |
| `highlightAnsi` | `(src: []const u8, lang: u8, dst: []u8) -> i64` | Render ANSI colored text (`-1` on overflow) |
| `highlightHtml` | `(src: []const u8, lang: u8, dst: []u8) -> i64` | Render XSS-escaped HTML `<span>` tags (`-1` on overflow) |

## Usage / 使用示例

```cp
import { LANG_CODE, highlightHtml } from "@cap/syntax-highlight";

let out: [512]u8 = undefined;
const n = highlightHtml("const x: i32 = 42;", LANG_CODE, out[0..512]);
```
