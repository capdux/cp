# ansi-regex — AI Agent Quick Ref

Zero-heap ANSI escape sequence matcher & locator reusing anstyle-parse, matching npm ansi-regex (400M+/wk) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    AnsiSpan,
    testAnsi,
    findFirstAnsi,
    findAllAnsi,
    countAnsiSequences,
    stripAllAnsi,
} from "ansi-regex";
```

## Core Types & Constants

```cp
class AnsiSpan { start: usize, len: usize, found: bool }
```

## API Signatures

```cp
fn testAnsi(input: string): bool;
fn findFirstAnsi(input: string): AnsiSpan;
fn findAllAnsi(input: string, out: []AnsiSpan): usize;
fn countAnsiSequences(input: string): usize;
fn stripAllAnsi(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { testAnsi } from "ansi-regex";

pub fn main() {
    let result = testAnsi("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ansi-regex"`; install via `cap add ansi-regex`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
