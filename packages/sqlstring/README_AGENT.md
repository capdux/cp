# sqlstring — AI Agent Quick Ref

Zero-allocation ANSI SQL / PostgreSQL / MySQL string literal, identifier, and LIKE pattern escaper with placeholder counter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    escapeQuoted,
    escapeStringAnsi,
    escapeIdentifierAnsi,
    escapeIdentifierMysql,
    escapeLike,
    isSafeUnquotedIdent,
    countPlaceholders,
} from "sqlstring";
```

## API Signatures

```cp
fn escapeQuoted(input: string, quote: u8, allow_empty: bool, out: []u8): usize;
fn escapeStringAnsi(input: string, out: []u8): usize;
fn escapeIdentifierAnsi(ident: string, out: []u8): usize;
fn escapeIdentifierMysql(ident: string, out: []u8): usize;
fn escapeLike(pattern: string, escape_char: u8, out: []u8): usize;
fn isSafeUnquotedIdent(ident: string): bool;
fn countPlaceholders(sql: string): usize;
```

## Minimal Usage

```cp
import { isSafeUnquotedIdent } from "sqlstring";

pub fn main() {
    let result = isSafeUnquotedIdent("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sqlstring"`; install via `cap add sqlstring`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
