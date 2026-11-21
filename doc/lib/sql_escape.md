# `cap:sql_escape` — SQL 标识符引用、字面量与 LIKE 模式安全转义库

> 来源：吸收自 `/data/dev/bun/src/sql/postgres/PostgresSQLQuery.zig`（SQL Identifier & Literal Escaper）

`cap:sql_escape` 提供纯 CP、零外部依赖、零堆分配的 PostgreSQL/ANSI 与 MySQL 双 SQL 方言标识符引用、字符串字面量转义以及 `LIKE` 通配符转义安全工具。

## 核心函数

```cp
pub fn escapeAnsiIdent(ident: []const u8, out: []u8): ([]const u8)?
pub fn escapeMysqlIdent(ident: []const u8, out: []u8): ([]const u8)?
pub fn escapeQualifiedAnsiIdent(qual_name: []const u8, out: []u8): ([]const u8)?
pub fn escapeSqlStringAnsi(val: []const u8, out: []u8): ([]const u8)?
pub fn escapeSqlStringMysql(val: []const u8, out: []u8): ([]const u8)?
pub fn escapeLikePattern(val: []const u8, esc_char: u8, out: []u8): ([]const u8)?
```

- `escapeAnsiIdent`：双引号包裹，内部 `"` 转义为 `""`；拒绝空串、`\0` 字节及缓冲区不足。
- `escapeMysqlIdent`：反引号包裹，内部 `` ` `` 转义为 `` `` ``；拒绝空串、`\0` 字节及缓冲区不足。
- `escapeQualifiedAnsiIdent`：按 `.` 切分并逐段转义限定标识符（如 `"public.users"` → `"\"public\".\"users\""`），拒绝空段与 `\0`。
- `escapeSqlStringAnsi`：单引号包裹，内部 `'` 转义为 `''`；拒绝 `\0` 字节。
- `escapeSqlStringMysql`：单引号包裹，转义 `\'`、`\"`、`\\`、`\n`、`\r`、`\0`、`\Z`（`0x1A`）。
- `escapeLikePattern`：使用指定 `esc_char`（如 `'\\'` 或 `'!'`）转义 `%`、`_` 与 `esc_char` 本身。

## 使用示例

```cp
import { escapeQualifiedAnsiIdent, escapeSqlStringAnsi, escapeLikePattern } from "cap:sql_escape";

let buf: [128]u8 = [0; 128];
const tbl = escapeQualifiedAnsiIdent("public.users", buf[0..]).?; // "\"public\".\"users\""
const lit = escapeSqlStringAnsi("O'Reilly", buf[0..]).?;          // "'O''Reilly'"
const pat = escapeLikePattern("50%_off\\deal", '\\', buf[0..]).?; // "50\\%\\_off\\\\deal"
```
