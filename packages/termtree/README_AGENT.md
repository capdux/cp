# termtree — AI Agent Quick Ref

Zero-allocation CLI tree structure renderer with Unicode/ASCII glyph palettes and fixed-capacity arena (Rust termtree parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    GlyphPalette,
    TermTreeNode,
    TermTreeArena,
    ttrAppendBytes,
    ttrAppendByte,
} from "termtree";
```

## Core Types & Constants

```cp
class GlyphPalette { middle_item: string, last_item: string, middle_skip: string, last_skip: string } // methods: unicode(...) +1
class TermTreeNode { label: string, first_child: i32, last_child: i32, next_sibling: i32 }
class TermTreeArena { nodes: [64]TermTreeNode, count: usize } // methods: init(...) +6
```

## API Signatures

```cp
fn ttrAppendBytes(out: []u8, pos: usize, src: string): usize?;
fn ttrAppendByte(out: []u8, pos: usize, b: u8): usize?;
```

## Minimal Usage

```cp
import { ttrAppendBytes } from "termtree";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ttrAppendBytes(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"termtree"`; install via `cap add termtree`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
