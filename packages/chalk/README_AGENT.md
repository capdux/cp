# chalk — AI Agent Quick Ref

Zero-allocation terminal string styling library (npm chalk v5 parity) with automatic color level detection & Truecolor/256/16/plain downsampling for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { Chalk, chkCopyPlain, chkWrapOpenClose, chkApplyPair } from "chalk";
```

## Core Types & Constants

```cp
class Chalk { level: u8 } // methods: new(...) +34
```

## API Signatures

```cp
fn chkCopyPlain(text: string, out: []u8): usize;
fn chkWrapOpenClose(open: string, text: string, close: string, out: []u8): usize;
fn chkApplyPair(level: u8, style: StylePair, text: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { chkCopyPlain } from "chalk";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = chkCopyPlain("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"chalk"`; install via `cap add chalk`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
