# range-parser — AI Agent Quick Ref

Zero-heap RFC 7233 HTTP Range header parser with suffix ranges and interval combining (clean-room port of npm range-parser) (`v1.0.0`, `MIT`)

## Import

```cp
import { ByteRange, RangeResult, parseRange, rangeByteLength } from "range-parser";
```

## Core Types & Constants

```cp
class ByteRange { start: u64, end: u64 }
class RangeResult { status: i32, unit: string, ranges: [32]ByteRange, count: usize }
```

## API Signatures

```cp
fn parseRange(size: u64, header: string, combine: bool): RangeResult;
fn rangeByteLength(r: ByteRange): u64;
```

## Minimal Usage

```cp
import { parseRange } from "range-parser";

pub fn main() {
    let result = parseRange(0, "example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"range-parser"`; install via `cap add range-parser`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
