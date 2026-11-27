# debugid — AI Agent Quick Ref

Zero-heap Sentry/Breakpad/PDB DebugId (128-bit UUID + appendix age) parser and formatter (`v0.8.0`, `Apache-2.0`)

## Import

```cp
import { DebugId, parseDebugId, formatBreakpad, formatCanonical } from "debugid";
```

## Core Types & Constants

```cp
class DebugId { bytes: [16]u8, appendix: u32, valid: bool } // methods: isNil(...)
```

## API Signatures

```cp
fn parseDebugId(s: string): DebugId;
fn formatBreakpad(id: DebugId, out: []u8): usize;
fn formatCanonical(id: DebugId, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseDebugId } from "debugid";

pub fn main() {
    let result = parseDebugId("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"debugid"`; install via `cap add debugid`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
