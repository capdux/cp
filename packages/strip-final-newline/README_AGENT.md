# strip-final-newline — AI Agent Quick Ref

Zero-copy final LF/CRLF newline stripper and normalizer matching npm strip-final-newline (250M+/wk) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    stripFinalNewline,
    stripAllTrailingNewlines,
    hasFinalNewline,
    hasFinalCrlf,
    ensureFinalNewline,
} from "strip-final-newline";
```

## API Signatures

```cp
fn stripFinalNewline(input: string): string;
fn stripAllTrailingNewlines(input: string): string;
fn hasFinalNewline(input: string): bool;
fn hasFinalCrlf(input: string): bool;
fn ensureFinalNewline(input: string, use_crlf: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { stripFinalNewline } from "strip-final-newline";

pub fn main() {
    let result = stripFinalNewline("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"strip-final-newline"`; install via `cap add strip-final-newline`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add strip-final-newline
cpc check src/main.cp
```
