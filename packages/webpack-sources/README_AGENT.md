# webpack-sources — AI Agent Quick Ref

Zero-heap source concatenation and range replacement engine (`v3.2.3`, `MIT`)

## Import

```cp
import {
    SourceEdit,
    ReplaceSource,
    wsrcWriteStr,
    wsrcEmptyEdit,
    concatSources,
} from "webpack-sources";
```

## Core Types & Constants

```cp
class SourceEdit { start: usize, end: usize, replacement: string }
class ReplaceSource { original: string, edits: [64]SourceEdit, edit_count: usize } // methods: init(...) +4
```

## API Signatures

```cp
fn wsrcWriteStr(out: []u8, pos: usize, s: string): usize;
fn wsrcEmptyEdit(): SourceEdit;
fn concatSources(parts: &[]string, sep: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { wsrcWriteStr } from "webpack-sources";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = wsrcWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"webpack-sources"`; install via `cap add webpack-sources`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
