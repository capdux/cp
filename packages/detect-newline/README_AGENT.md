# detect-newline — AI Agent Quick Ref

Zero-allocation dominant newline detector and cross-platform LF/CRLF normalizer for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    NewlineStats,
    analyzeNewlines,
    detectNewline,
    detectNewlineGraceful,
    normalizeToLf,
    normalizeToCrlf,
} from "detect-newline";
```

## Core Types & Constants

```cp
class NewlineStats { lf_count: usize, crlf_count: usize, cr_count: usize, dominant: u8 }
```

## API Signatures

```cp
fn analyzeNewlines(input: string): NewlineStats;
fn detectNewline(input: string): string;
fn detectNewlineGraceful(input: string): string;
fn normalizeToLf(input: string, out: []u8): usize;
fn normalizeToCrlf(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { analyzeNewlines } from "detect-newline";

pub fn main() {
    let result = analyzeNewlines("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"detect-newline"`; install via `cap add detect-newline`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
