# magic-string — AI Agent Quick Ref

Zero-allocation original-offset source string editor (npm magic-string parity) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MstrChunk,
    MstrInsert,
    MagicString,
    mstrWriteStr,
    mstrWriteLeftInserts,
    mstrWriteRightInserts,
} from "magic-string";
```

## Core Types & Constants

```cp
class MstrChunk { start: usize, end: usize, content: string, edited: bool }
class MstrInsert { index: usize, content: string }
class MagicString { original: string, changed: bool, chunks: [64]MstrChunk, chunk_count: usize, /* +8 fields */ } // methods: new(...) +11
```

## API Signatures

```cp
fn mstrWriteStr(src: string, out: []u8, pos: usize): usize;
fn mstrWriteLeftInserts(inserts: [32]MstrInsert, count: usize, target_idx: usize, out: []u8, pos: usize): usize;
fn mstrWriteRightInserts(inserts: [32]MstrInsert, count: usize, target_idx: usize, out: []u8, pos: usize): usize;
```

## Minimal Usage

```cp
import { mstrWriteStr } from "magic-string";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = mstrWriteStr("example", out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"magic-string"`; install via `cap add magic-string`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
