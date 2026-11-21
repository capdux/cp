# `cap:toml_lexer` — 纯 CP 零堆分配 TOML v1.0 词法扫描器

## 概述

`cap:toml_lexer` 吸收自 Bun `src/interchange/toml/lexer.cp`，纯 CP 零外部依赖、零堆分配，完整支持 TOML v1.0 表/数组表、内联表、点分键、四种字符串、四进制+下划线整型、浮点/`inf`/`nan`、布尔、ISO 8601 日期时间及精确行列号追踪。

## 核心类型与 API

- `TomlTokenKind`：21 种词法类型（`eof`/`newline`/`equals`/`dot`/`comma`/`lbracket`/`rbracket`/`double_lbracket`/`double_rbracket`/`lbrace`/`rbrace`/`bare_key`/`basic_string`/`multiline_basic_string`/`literal_string`/`multiline_literal_string`/`integer`/`float_val`/`boolean`/`datetime`/`invalid`）
- `TomlToken`：`kind`、`raw`、`start`、`end`、`line`、`col`、`int_val: i64`、`bool_val: bool`
- `TomlLexer`：`init(src: []const u8)`、`next(*_): TomlToken`

## 示例

```cp
import { TomlLexer, TomlTokenKind } from "cap:toml_lexer";

let lex = TomlLexer.init("[package]\nname = \"cp\"\nport = 0x1F_90\n");
while (true) {
    const tok = lex.next();
    if (tok.kind == TomlTokenKind.eof) { break; }
}
```
