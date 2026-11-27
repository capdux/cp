# shlex — AI Agent Quick Ref

Zero-allocation POSIX shell quoting, command-line lexical splitting & injection guard (shell-quote / Rust shlex parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ShlexError,
    SplitResult,
    isSafeUnquotedChar,
    needsQuote,
    hasShellMetachars,
    appendByte,
    quoteArg,
    joinArgs,
} from "shlex";
```

## Core Types & Constants

```cp
enum ShlexError { None, UnclosedSingleQuote, UnclosedDoubleQuote, TrailingBackslash, TooManyTokens, BufferOverflow }
class SplitResult { pool: [MAX_POOL_BYTES]u8, starts: [MAX_TOKENS]usize, lens: [MAX_TOKENS]usize, count: usize, /* +2 fields */ } // methods: empty(...) +1
```

## API Signatures

```cp
fn isSafeUnquotedChar(c: u8): bool;
fn needsQuote(arg: string): bool;
fn hasShellMetachars(s: string): bool;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn quoteArg(arg: string, out: []u8): string;
fn joinArgs(args: []string, out: []u8): string;
fn splitShell(cmd: string): SplitResult;
```

## Minimal Usage

```cp
import { needsQuote } from "shlex";

pub fn main() {
    let result = needsQuote("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"shlex"`; install via `cap add shlex`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
