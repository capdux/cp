[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# sql-formatter

Zero-allocation SQL pretty-printer, keyword uppercasing normalizer, and minifier for CP — clean-room MIT parity with npm `sql-formatter` and Rust `sqlformat`.

Lexer-aware formatting protects single-quoted strings (`'...'` with `''` / `\'` escapes), quoted identifiers (`"..."`, `` `...` ``), line comments (`-- ...`), and block comments (`/* ... */`) while laying out clauses (`SELECT`, `FROM`, `LEFT/INNER/RIGHT JOIN`, `WHERE`, `GROUP BY`, `ORDER BY`, `HAVING`, `LIMIT`, `INSERT INTO`, `VALUES`, `UPDATE`, `SET`, `DELETE FROM`, `UNION ALL`) directly into caller-provided stack buffers.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `sql-formatter` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/sql-formatter-org/sql-formatter` |
| License | MIT |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: sql-formatter)
import { format } from "sql-formatter";
const pretty = format("select id, name from users where active = 1");
```

```cp
// CP (cap: sql-formatter) — zero-allocation stack-buffered SQL formatter
import { formatSql } from "sql-formatter";

let buf: [2048]u8 = undefined;
const pretty = formatSql("select id, name from users where active = 1", buf[0..2048]);
```

## Installation

```bash
cap add sql-formatter
```

## Usage

### Basic Usage: Pretty-Print & Minify SQL

```cp
import { formatSql, minifySql } from "sql-formatter";

fn demoSql() {
    let b1: [2048]u8 = undefined;
    let b2: [2048]u8 = undefined;

    const raw = "select u.id, u.name, count(o.id) as cnt from users u left join orders o on u.id = o.user_id where u.active = 1 and o.total >= 100 group by u.id, u.name order by cnt desc limit 10;";
    const pretty = formatSql(raw, b1[0..2048]);

    // Strip comments and collapse whitespace while preserving string literals verbatim
    const compact = minifySql("-- audit query\nSELECT  id,  name  FROM  users  WHERE  note = 'a  b';", b2[0..2048]);
    // => "SELECT id, name FROM users WHERE note = 'a  b';"
    _ = pretty;
    _ = compact;
}
```

### Edge Cases: Custom Options, String/Comment Safety & Multi-Query Spacing

```cp
import { SqlFormatOptions, formatSqlWith } from "sql-formatter";

fn demoCustom() {
    let buf: [2048]u8 = undefined;

    let opts = SqlFormatOptions.defaults();
    opts.indent_width = 4;
    opts.uppercase_keywords = false;   // keep original keyword casing
    opts.lines_between_queries = 2;    // blank lines between semicolon-separated statements

    // Keywords inside string literals ('select from') are never uppercased or broken
    const sql = "select 'select from' as txt from logs; delete from temp where id = 1;";
    const out = formatSqlWith(sql, opts, buf[0..2048]);

    // Small output buffer truncates safely without panic or memory corruption
    let tiny: [16]u8 = undefined;
    const trunc = formatSqlWith(sql, opts, tiny[0..16]);
    _ = out;
    _ = trunc;
}
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `SqlFormatOptions` | `class { indent_width: u8, uppercase_keywords: bool, lines_between_queries: u8 }` | Formatting configuration with `SqlFormatOptions.defaults()` (`indent_width = 2`, `uppercase_keywords = true`, `lines_between_queries = 1`). |
| `formatSql` | `fn(sql: string, out: []u8): string` | Formats `sql` using default options into `out`. |
| `formatSqlWith` | `fn(sql: string, opts: SqlFormatOptions, out: []u8): string` | Formats `sql` using custom `SqlFormatOptions` into `out`. |
| `minifySql` | `fn(sql: string, out: []u8): string` | Strips SQL comments and collapses whitespace into a compact single line while preserving string literals. |
| `nextToken` | `fn(sql: string, start_pos: usize): Token` | Low-level zero-copy SQL lexer returning `{ kind: u8, start: usize, end: usize }`. |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`isSpace`, `isDigit`, `isAlpha`, `toUpperByte`, `eqIgnoreCase`) and `cap:fmt` (`writeStr`) for fast single-pass lexing and uppercasing.
- **Zero Heap Allocation**: Operates in a single pass with zero AST heap allocations, writing directly into caller-provided `out: []u8`.
- **Zero-Clear Caller Buffers (`undefined`)**: Callers can pass stack buffers initialized with `undefined`; every byte write is bounds-checked against `out.len`.
- **Lexer-Aware Safety**: String literals, quoted identifiers, and comments are never corrupted by keyword uppercasing or whitespace normalization.

## License

MIT — Clean-room CP implementation inspired by npm `sql-formatter`.
