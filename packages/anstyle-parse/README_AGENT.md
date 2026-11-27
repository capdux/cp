# anstyle-parse — AI Agent Quick Ref

Zero-heap Paul Williams VT100/ANSI escape sequence state-machine parser (CSI/OSC/ESC/SGR) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    AnsiToken,
    SgrParams,
    nextToken,
    tokenizeAnsi,
    parseCsiParams,
    stripAnsiToBuf,
    hasAnsi,
    visibleByteLen,
} from "anstyle-parse";
```

## Core Types & Constants

```cp
class AnsiToken { kind: u8, start: usize, len: usize, final_byte: u8 }
class SgrParams { params: [16]u16, count: usize, valid: bool }
```

## API Signatures

```cp
fn nextToken(input: string, pos: usize): AnsiToken;
fn tokenizeAnsi(input: string, out: []AnsiToken): usize;
fn parseCsiParams(csi_seq: string): SgrParams;
fn stripAnsiToBuf(input: string, out: []u8): usize;
fn hasAnsi(input: string): bool;
fn visibleByteLen(input: string): usize;
```

## Minimal Usage

```cp
import { parseCsiParams } from "anstyle-parse";

pub fn main() {
    let result = parseCsiParams("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"anstyle-parse"`; install via `cap add anstyle-parse`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
