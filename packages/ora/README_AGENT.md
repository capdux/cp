# ora — AI Agent Quick Ref

Zero-heap CLI terminal spinner animation and status line renderer (clean-room port of ora v8) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Spinner,
    oraWriteSlice,
    oraFrameCount,
    oraFrameAt,
    oraRenderStatus,
} from "ora";
```

## Core Types & Constants

```cp
class Spinner { style: u8, text: string, frame_index: usize } // methods: new(...) +11
```

## API Signatures

```cp
fn oraWriteSlice(out: []u8, pos: usize, s: string): usize;
fn oraFrameCount(style: u8): usize;
fn oraFrameAt(style: u8, index: usize): string;
fn oraRenderStatus(symbol: string, msg: string, fallback: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { oraWriteSlice } from "ora";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = oraWriteSlice(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ora"`; install via `cap add ora`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
