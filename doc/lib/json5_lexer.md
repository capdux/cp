# `cap:json5_lexer` — 零堆分配 JSON5 词法扫描器与标准 JSON 规范化器

> 来源：吸收自 `/data/dev/bun/src/interchange/json5.cp`

`cap:json5_lexer` 提供纯 CP、零外部依赖、零堆分配的 JSON5 词法扫描器（`Json5Lexer`）与标准 RFC 8259 JSON 转译器（`toStandardJson`），完整支持单/多行注释、尾随逗号、单引号字符串、续行反斜杠、无引号 ES5 键名、十六进制数字、前导/尾随小数点与 `Infinity`/`NaN`。

## 核心类型

### `Json5TokenKind`

```cp
pub enum Json5TokenKind: u8 {
    eof = 0,
    lbrace = 1,
    rbrace = 2,
    lbracket = 3,
    rbracket = 4,
    colon = 5,
    comma = 6,
    string = 7,
    number = 8,
    boolean = 9,
    null_lit = 10,
    identifier = 11,
    invalid = 12,

    fn asStr(): string
}
```

> 提示：由于 `string` 是 CP 内建类型关键字，在表达式中引用 `string` 变体请使用全限定名 `Json5TokenKind.string`。

### `Json5Token`

```cp
pub struct Json5Token {
    kind: Json5TokenKind = .eof,
    raw: []const u8 = "",
    start: usize = 0,
    end: usize = 0,
    num_val: f64 = 0.0,
    bool_val: bool = false,
}
```

### `Json5Lexer`

```cp
pub struct Json5Lexer {
    src: []const u8,
    pos: usize = 0,

    static fn init(src: []const u8): Json5Lexer
    fn next(*_): Json5Token
}
```

- 自动跳过空白、UTF-8 BOM/NBSP、`//...` 单行注释与 `/* ... */` 多行注释；未闭合多行注释返回 `.invalid`。
- 支持双引号与单引号字符串（含 `\"`、`\'`、`\\` 与 `\\\n` / `\\\r\n` 续行反斜杠）；未闭合字符串或裸换行返回 `.invalid`。
- 支持十进制（`+42`、`.5`、`5.`、`1e-3`）、十六进制（`0x1A`、`-0xFF`）、`Infinity` / `-Infinity` / `NaN`。
- 支持 `true` / `false` / `null` 与无引号 ES5 标识符（`$foo_1`）。

### `toStandardJson`

```cp
pub fn toStandardJson(src: []const u8, out: []u8): ([]const u8)?
```

- 将合法 JSON5 文本零堆分配转译为标准 RFC 8259 JSON：
  - 移除单行与多行注释；
  - 移除对象与数组末尾的尾随逗号；
  - 将单引号字符串与无引号键名规范化为双引号字符串（自动转义内部 `"`、去除续行反斜杠）；
  - 将十六进制转为十进制整数，规范化 `+3` → `3`、`.5` → `0.5`、`5.` → `5`；
  - 对非有限浮点（`Infinity`/`NaN`）、语法错误、超深嵌套（>64 层）或输出缓冲区溢出返回 `null`。

## 使用示例

```cp
import { Json5Lexer, Json5TokenKind, toStandardJson } from "cap:json5_lexer";

const src = "{ /* config */ $host: 'localhost', port: 0x1F90, ratio: .5, }";
let out: [256]u8 = [0; 256];
const std_json = toStandardJson(src, out[0..]).?;
// std_json == "{\"$host\":\"localhost\",\"port\":8080,\"ratio\":0.5}"
```
