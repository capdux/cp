# `cap:json5` — 零分配 JSON5 1.0 词法器、校验器与标准 JSON 转换器

> 参考来源：/data/dev/bun/src/interchange/json5.cp | 吸收日期：2026-11-22

`cap:json5` 提供完整的 JSON5 1.0 词法迭代、语法校验以及零堆分配的标准 RFC 8259 JSON 转换能力，适用于配置文件（如 `tsconfig.json` / `cap.json5`）、高吞吐预处理管线与无 GC 嵌入式场景。

## 核心特性

- **注释支持**：自动忽略 `//` 单行注释与 `/* ... */` 多行块注释（检测未闭合块注释）。
- **字符串扩展**：支持单引号 `'...'` 与双引号 `"..."`、多行续行符（`\` + `\n`/`\r\n`/`\r`）、`\xHH`、`\v`、`\0` 及标准 `\uHHHH` 转义，转换标准 JSON 时自动规范转义。
- **对象键与尾随逗号**：支持未加引号的合法标识符键（含 `$`、`_` 及关键字标识符名）与对象/数组末尾尾随逗号，转换标准 JSON 时自动补双引号并移除尾随逗号。
- **扩展数值**：支持十六进制整数（`0xFF` / `-0x1A`）、前导小数点（`.5` / `-.5`）、尾随小数点（`5.` / `5.e2`）、显式 `+` 号以及 `Infinity` / `-Infinity` / `NaN`（转严格 RFC 8259 JSON 时规范输出为 `null`）。
- **零分配与安全防护**：全程在调用方提供的 `[]u8` 缓冲区内完成转换，具备严格缓冲越界检测与 64 层最大嵌套深度防护。

## API 参考

### `Json5TokenKind`

```cp
pub enum Json5TokenKind: u8 {
    lbrace, rbrace, lbracket, rbracket,
    colon, comma, string, ident, number,
    true_lit, false_lit, null_lit, invalid,
}
```

### `Json5Token`

```cp
pub struct Json5Token {
    kind: Json5TokenKind,
    lexeme: []const u8,
}
```

### `Json5Iter`

- `Json5Iter.init(input: []const u8): Json5Iter` — 创建零拷贝 JSON5 词法迭代器。
- `iter.next(*_): Json5Token?` — 跳过空白与注释后返回下一个 token，输入结束返回 `null`，遇到非法词法返回 `.invalid`。

### `toStandardJson(input: []const u8, out: []u8): ([]const u8)?`

- 将 JSON5 文本零堆分配转换为严格 RFC 8259 JSON 切片（位于 `out` 内）。
- 若输入不合法、嵌套超过 64 层、十六进制溢出 `u64` 或 `out` 缓冲区不足，返回 `null`。

### `isValidJson5(input: []const u8): bool`

- 零分配校验输入是否为合法的 JSON5 1.0 文档。

## 使用示例

```cp
import { Json5Iter, toStandardJson, isValidJson5 } from "cap:json5";

const src = "{ // config\n  unquoted: 'hello \"world\"', hex: 0xFF, lead: .5, trail: 5., plus: +42, arr: [1, 2,], }";
let buf: [256]u8 = [0; 256];
if (json := toStandardJson(src, buf[0..])) {
    // {"unquoted":"hello \"world\"","hex":255,"lead":0.5,"trail":5.0,"plus":42,"arr":[1,2]}
    * = json;
}
```

## 上游行为契约与规范化边界

1. **JSON5 到 RFC 8259 JSON 严格降级规则 (`toStandardJson`)**：
   - `Infinity` / `-Infinity` / `NaN` 按 RFC 8259 兼容性规范化为 `null`；前导/尾随小数点（`.5` / `5.`）自动补 `0`（`0.5` / `5.0`）；显式 `+` 号剥离；单引号字符串内的未转义 `"` 自动转义为 `\"`。
2. **嵌套深度与整数溢出防护**：
   - 严格限制最大嵌套深度 64 层以防止畸形配置栈溢出；十六进制整数超出 `u64` 或遇到未闭合块注释 `/*` 时返回 `null`；建议调用方预留 `out.len >= input.len * 2` 以容纳补全双引号与转义展开。
