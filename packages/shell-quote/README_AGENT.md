# shell-quote — AI Agent Quick Ref

Zero-heap POSIX shell argument quoting and command-line tokenization in pure CP (modeled after npm shell-quote) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParsedArgSpan,
    SplitResult,
    isSafeBareChar,
    needsQuoting,
    quoteArg,
    quoteArgs,
    splitCommand,
} from "shell-quote";
```

## Core Types & Constants

```cp
class ParsedArgSpan { start: usize, len: usize }
class SplitResult { arg_count: usize, bytes_used: usize, ok: bool }
```

## API Signatures

```cp
fn isSafeBareChar(c: u8): bool;
fn needsQuoting(arg: &[]u8): bool;
fn quoteArg(arg: &[]u8, dst: []u8): i64;
fn quoteArgs(args: &[]&[]u8, dst: []u8): i64;
fn splitCommand(cmd: &[]u8, out_buf: []u8, out_spans: []ParsedArgSpan): SplitResult;
```

## Minimal Usage

```cp
import { isSafeBareChar } from "shell-quote";

pub fn main() {
    let result = isSafeBareChar(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"shell-quote"`; install via `cap add shell-quote`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
