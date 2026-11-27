# snakecase-keys — AI Agent Quick Ref

Zero-heap object key converter to snake_case with exclude list and fixed key buffer pool, composing map-obj and heck-case (clean-room port of npm snakecase-keys) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SnakecaseKeysResult,
    sckStrEq,
    sckIsExcluded,
    snakecaseKeyInto,
    snakecaseKeys,
} from "snakecase-keys";
```

## Core Types & Constants

```cp
class SnakecaseKeysResult { key_buf: [1024]u8, key_buf_len: usize, key_offs: [32]usize, key_lens: [32]usize, /* +3 fields */ } // methods: empty(...) +4
```

## API Signatures

```cp
fn sckStrEq(a: string, b: string): bool;
fn sckIsExcluded(k: string, exclude: []string): bool;
fn snakecaseKeyInto(key: string, out: []u8): usize;
fn snakecaseKeys(src: MapObjResult, exclude: []string): SnakecaseKeysResult;
```

## Minimal Usage

```cp
import { sckStrEq } from "snakecase-keys";

pub fn main() {
    let result = sckStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"snakecase-keys"`; install via `cap add snakecase-keys`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
