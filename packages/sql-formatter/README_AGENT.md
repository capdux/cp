# sql-formatter@1.0.0 — AI Agent Quick Ref

> Zero-alloc SQL pretty-printer, keyword uppercaser & minifier | Source: `npm:sql-formatter` | License: MIT
> Import: `import { SqlFormatOptions, formatSql, formatSqlWith, minifySql } from "sql-formatter";` | Types: `src/index.d.cp`

## Core Types

```cp
export class SqlFormatOptions {
    indent_width: u8,          // default: 2
    uppercase_keywords: bool,  // default: true
    lines_between_queries: u8, // default: 1
    fn defaults(): SqlFormatOptions;
}

export class Token {
    kind: u8, start: usize, end: usize,
}
```

## Core Functions

- `formatSql(sql: string, out: []u8): string` — Format with `SqlFormatOptions.defaults()`.
- `formatSqlWith(sql: string, opts: SqlFormatOptions, out: []u8): string` — Format with custom options.
- `minifySql(sql: string, out: []u8): string` — Strip `--` / `/* */` comments & collapse whitespace; preserves `'...'` strings verbatim.
- `nextToken(sql: string, start_pos: usize): Token` — Zero-copy single-token lexer.
- `isSqlKeyword(sql: string, s: usize, e: usize): bool` | `isSqlFunctionKeyword(sql: string, s: usize, e: usize): bool`

## Minimal Usage

```cp
import { SqlFormatOptions, formatSql, formatSqlWith, minifySql } from "sql-formatter";

let buf: [2048]u8 = undefined;
const pretty = formatSql("select id, name from users where active = 1", buf[0..2048]);

let opts = SqlFormatOptions.defaults();
opts.indent_width = 4;
const pretty4 = formatSqlWith("select a, b from t", opts, buf[0..2048]);
const compact = minifySql("-- note\nSELECT  a,  b  FROM  t;", buf[0..2048]);
```

## Pitfalls & Rules

1. **Buffer sizing**: Pretty-printing adds newlines and indentation; size `out` to at least `2x` input length to avoid truncation.
2. **Buffer aliasing**: Returned `string` slices `out`; do not reuse the same `out` buffer if keeping both `pretty` and `compact` alive simultaneously.
3. **Safe truncation**: If `out` is too small, output is cleanly truncated at `out.len` without panic.
4. **String & comment safety**: `'...'` (with `''`/`\'`), `"..."`, `` `...` ``, `-- ...`, and `/* ... */` are lexed atomically and never corrupted by keyword uppercasing.
5. **Minify removes comments**: `minifySql` intentionally strips `--` line comments and `/* */` block comments.
