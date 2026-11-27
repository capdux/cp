# eventsource-parser — AI Agent Quick Ref

Zero-allocation W3C Server-Sent Events (text/event-stream) incremental streaming parser for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import { SseParser, sseBytesEq } from "eventsource-parser";
```

## Core Types & Constants

```cp
class SseParser { data_buf: [2048]u8, data_len: usize, event_buf: [128]u8, event_len: usize, /* +7 fields */ } // methods: init(...) +8
```

## API Signatures

```cp
fn sseBytesEq(a: &[]u8, b: &[]u8): bool;
```

## Minimal Usage

```cp
import { sseBytesEq } from "eventsource-parser";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = sseBytesEq(out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"eventsource-parser"`; install via `cap add eventsource-parser`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add eventsource-parser
cpc check src/main.cp
```
