# decamelize-keys — AI Agent Quick Ref

Zero-heap object key decamelizer with custom separator, exclude list, and fixed key buffer pool, composing map-obj and decamelize (clean-room port of npm decamelize-keys) (`v1.0.0`, `MIT`)

## Import

```cp
import { DecamelizeKeysResult, dckStrEq, dckIsExcluded, decamelizeKeys } from "decamelize-keys";
```

## Core Types & Constants

```cp
class DecamelizeKeysResult { key_buf: [1024]u8, key_buf_len: usize, key_offs: [32]usize, key_lens: [32]usize, /* +3 fields */ } // methods: empty(...) +4
```

## API Signatures

```cp
fn dckStrEq(a: string, b: string): bool;
fn dckIsExcluded(k: string, exclude: []string): bool;
fn decamelizeKeys(src: MapObjResult, sep: string, exclude: []string): DecamelizeKeysResult;
```

## Minimal Usage

```cp
import { dckStrEq } from "decamelize-keys";

pub fn main() {
    let result = dckStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"decamelize-keys"`; install via `cap add decamelize-keys`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
