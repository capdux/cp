[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sql-formatter

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation SQL pretty-printer, keyword uppercasing normalizer, and minifier (sql-formatter / Rust sqlformat parity) for CP

## 安装与导入

```bash
cap add sql-formatter
```

```cp
import { isSpace } from "sql-formatter";
```

## 核心导出 API

- `export fn isSpace(c: u8): bool;`
- `export fn isAlpha(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn isWordCont(c: u8): bool;`
- `export fn toUpperAscii(c: u8): u8;`
- `export fn eqIgnoreCase(sql: string, start: usize, end: usize, upper: string): bool;`
- `export fn nextToken(sql: string, start_pos: usize): Token;`
- `export fn isSqlFunctionKeyword(sql: string, s: usize, e: usize): bool;`
- `export fn isSqlKeyword(sql: string, s: usize, e: usize): bool;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushSlice(out: []u8, pos: usize, src: string, start: usize, end: usize, upper: bool): usize;`
- `export fn pushIndent(out: []u8, pos: usize, level: usize, indent_width: u8): usize;`
- `export fn pushNewlineIndent(out: []u8, pos: usize, level: usize, indent_width: u8): usize;`
- `export fn isJoinLeadWord(sql: string, s: usize, e: usize): bool;`
- `export fn isTopClauseWord(sql: string, s: usize, e: usize): bool;`
- `export fn formatSql(sql: string, out: []u8): string;`
- `export fn formatSqlWith(sql: string, opts: SqlFormatOptions, out: []u8): string;`
- `export fn isTightDelimiter(kind: u8): bool;`
- `export fn minifySql(sql: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`sql-formatter@1.0.0`)，许可证：MIT
