# ms — AI Agent Quick Ref

Zero-heap millisecond conversion utility (clean-room CP implementation of npm ms) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MsParseResult,
    msUnitMultiplier,
    parseMs,
    msRoundDiv,
    msFormatUnit,
    formatMs,
} from "ms";
```

## Core Types & Constants

```cp
class MsParseResult { ok: bool, ms: i64 }
```

## API Signatures

```cp
fn msUnitMultiplier(s: string, start: usize, end: usize): f64;
fn parseMs(input: string): MsParseResult;
fn msRoundDiv(ms: i64, unit: i64): i64;
fn msFormatUnit(val: i64, short_unit: string, long_singular: string, long_plural: string, long_fmt: bool, out: []u8): usize;
fn formatMs(ms: i64, long_fmt: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseMs } from "ms";

pub fn main() {
    let result = parseMs("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ms"`; install via `cap add ms`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
