[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sqlstring

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ANSI SQL / PostgreSQL / MySQL string literal, identifier, and LIKE pattern escaper with placeholder counter in pure CP

## 安装与导入

```bash
cap add sqlstring
```

```cp
import { escapeQuoted } from "sqlstring";
```

## 核心导出 API

- `export fn escapeQuoted(input: string, quote: u8, allow_empty: bool, out: []u8): usize;`
- `export fn escapeStringAnsi(input: string, out: []u8): usize;`
- `export fn escapeIdentifierAnsi(ident: string, out: []u8): usize;`
- `export fn escapeIdentifierMysql(ident: string, out: []u8): usize;`
- `export fn escapeLike(pattern: string, escape_char: u8, out: []u8): usize;`
- `export fn isAlphaOrUnder(b: u8): bool;`
- `export fn isIdentRest(b: u8): bool;`
- `export fn isSafeUnquotedIdent(ident: string): bool;`
- `export fn countPlaceholders(sql: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`sqlstring@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`pg-format@1.0.0`)，许可证：MIT
