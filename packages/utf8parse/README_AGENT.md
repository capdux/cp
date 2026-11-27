# utf8parse — AI Agent Quick Ref

Zero-allocation RFC 3629 strict UTF-8 validator, WHATWG lossy repair (U+FFFD), ASCII fast-path, and UTF-16 code-unit length calculator for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Utf8Error,
    Utf8CheckResult,
    StepResult,
    isCont,
    checkSeqAt,
    isAscii,
    validateUtf8,
    isValidUtf8,
} from "utf8parse";
```

## Core Types & Constants

```cp
class Utf8Error { valid_up_to: usize, error_len: u8 }
class Utf8CheckResult { valid: bool, valid_up_to: usize, error_len: u8, codepoint_count: usize }
class StepResult { valid: bool, step_len: usize, error_len: u8 }
```

## API Signatures

```cp
fn isCont(b: u8): bool;
fn checkSeqAt(data: string, pos: usize): StepResult;
fn isAscii(data: string): bool;
fn validateUtf8(data: string): Utf8CheckResult;
fn isValidUtf8(data: string): bool;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn toLossyUtf8(data: string, out: []u8): string;
fn utf16Len(data: string): usize;
```

## Minimal Usage

```cp
import { isAscii } from "utf8parse";

pub fn main() {
    let result = isAscii("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"utf8parse"`; install via `cap add utf8parse`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
