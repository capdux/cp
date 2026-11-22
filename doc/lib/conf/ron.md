# `cap:ron` — 零分配 RON (Rusty Object Notation) 词法扫描、平衡校验与字段提取标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:conf`，导入请使用 `import { ... } from "cap:conf"`。

`cap:ron` 提供零堆分配的 `.ron` 词法流式扫描、括号结构平衡与深度校验以及顶层字段零拷贝值提取能力，完整支持嵌套块注释、原始字符串与多进制下划线数字。

## 核心 API

### 词法类型与迭代器
- `RonTokenKind`：`lparen`, `rparen`, `lbracket`, `rbracket`, `lbrace`, `rbrace`, `colon`, `comma`, `ident`, `string`, `char_lit`, `number`, `bool_lit`, `invalid`
- `RonToken`：`kind: RonTokenKind`, `lexeme: []const u8`
- `RonIter`：
  - `RonIter.init(input: []const u8): RonIter` — 自动跳过 UTF-8 BOM
  - `next(*_): RonToken?` — 跳过 `//` 行注释与可嵌套 `/* ... */` 块注释，支持双引号转义字符串、原始字符串 `r#"..."#`、字符字面量 `'a'`/`'\n'`、多进制下划线数字（`1_000`、`0xff`、`0b1010`、`0o755`）、标识符与 `true`/`false`

### 校验与字段提取
- `validateRon(input: []const u8): bool` — 栈式校验 `()`/`[]`/`{}` 匹配平衡、最大 64 层嵌套深度、注释与字符串闭合、无非法 token 且至少含 1 个 token
- `extractRonField(input: []const u8, field_name: []const u8): ([]const u8)?` — 零分配提取顶层 `field_name: <value>` 原始切片，自动跳过嵌套结构与字符串内的冒号/逗号

## 使用示例

```cp
import { RonIter, validateRon, extractRonField } from "cap:conf";

fn demo(src: []const u8) {
    if (!validateRon(src)) {
        return;
    }
    if (port := extractRonField(src, "port")) {
        * = port;
    }
    let it = RonIter.init(src);
    while (tok := it.next()) {
        * = tok.lexeme;
    }
}
```
