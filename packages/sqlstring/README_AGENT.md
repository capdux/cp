# sqlstring@1.0.0 (Agent Quickref)

> Zero-allocation ANSI SQL / PostgreSQL / MySQL string literal, identifier, and LIKE pattern escaper with placeholder counter in pure CP | License: MIT
> Import: `import { ... } from "sqlstring";` | Full types: `src/index.d.cp`

## Functions
- `export fn escapeQuoted(input: string, quote: u8, allow_empty: bool, out: []u8): usize;`
- `export fn escapeStringAnsi(input: string, out: []u8): usize;`
- `export fn escapeIdentifierAnsi(ident: string, out: []u8): usize;`
- `export fn escapeIdentifierMysql(ident: string, out: []u8): usize;`
- `export fn escapeLike(pattern: string, escape_char: u8, out: []u8): usize;`
- `export fn isAlphaOrUnder(b: u8): bool;`
- `export fn isIdentRest(b: u8): bool;`
- `export fn isSafeUnquotedIdent(ident: string): bool;`
- `export fn countPlaceholders(sql: string): usize;`
