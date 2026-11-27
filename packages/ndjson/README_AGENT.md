# ndjson — AI Agent Quick Ref

Zero-heap NDJSON / JSON Lines streaming incremental line splitter, scanner, and writer with max-line overflow protection. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    NdjsonLine,
    NdjsonSplitter,
    isBlankSlice,
    nextLine,
    countLines,
    writeLine,
} from "ndjson";
```

## Core Types & Constants

```cp
class NdjsonLine { start: usize, len: usize, found: bool }
class NdjsonSplitter { buf: [1024]u8, pos: usize, ready_len: usize, overflowed: bool } // methods: init(...) +4
```

## API Signatures

```cp
fn isBlankSlice(s: &[]u8): bool;
fn nextLine(src: &[]u8, start_pos: usize, out_next_pos: &mut usize): NdjsonLine;
fn countLines(src: &[]u8): usize;
fn writeLine(json_record: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { isBlankSlice } from "ndjson";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isBlankSlice(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ndjson"`; install via `cap add ndjson`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
