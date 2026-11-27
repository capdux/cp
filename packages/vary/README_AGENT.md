# vary — AI Agent Quick Ref

Zero-heap RFC 9110 HTTP Vary header manipulation, deduplication, case-insensitive lookup, token validation, and wildcard collapsing. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isWs,
    isTchar,
    isValidFieldName,
    isWildcard,
    hasField,
    headerHasWildcard,
    validateFieldList,
    dstContainsToken,
} from "vary";
```

## API Signatures

```cp
fn isWs(c: u8): bool;
fn isTchar(c: u8): bool;
fn isValidFieldName(field: &[]u8): bool;
fn isWildcard(tok: &[]u8): bool;
fn hasField(header: &[]u8, field: &[]u8): bool;
fn headerHasWildcard(src: &[]u8): bool;
fn validateFieldList(field_list: &[]u8): bool;
fn dstContainsToken(dst: &[]u8, written: usize, tok: &[]u8): bool;
fn appendTokens(src: &[]u8, dst: []u8, start_pos: usize): i64;
fn append(header: &[]u8, field: &[]u8, dst: []u8): i64;
fn appendMany(header: &[]u8, fields: &[]&[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { isWs } from "vary";

pub fn main() {
    let result = isWs(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"vary"`; install via `cap add vary`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
