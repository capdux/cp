# error-stack-parser — AI Agent Quick Ref

Zero-allocation multi-language stack trace parser & cleaner supporting V8/Node/Bun, SpiderMonkey/Safari, and Rust/Zig/CP formats (error-stack-parser / stacktrace-parser / clean-stack parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StackFrame,
    ParsedStack,
    LocationInfo,
    isInternalLocation,
    parseLocation,
    parseFrameLine,
    parseStack,
    pushByte,
} from "error-stack-parser";
```

## Core Types & Constants

```cp
class StackFrame { func_name: string, file: string, line: u32, column: u32, /* +2 fields */ } // methods: empty(...)
class ParsedStack { frames: [MAX_FRAMES]StackFrame, count: usize, message: string } // methods: empty(...) +1
class LocationInfo { file: string, line: u32, column: u32, is_native: bool }
```

## API Signatures

```cp
fn isInternalLocation(file: string, is_native: bool): bool;
fn parseLocation(raw_loc: string): LocationInfo;
fn parseFrameLine(raw_line: string): StackFrame?;
fn parseStack(raw: string): ParsedStack;
fn pushByte(out: []u8, pos: usize, b: u8): usize;
fn formatFrameAt(f: StackFrame, out: []u8, start_pos: usize): usize;
fn formatFrame(f: StackFrame, out: []u8): string;
fn cleanStack(raw: string, out: []u8): string;
```

## Minimal Usage

```cp
import { isInternalLocation } from "error-stack-parser";

pub fn main() {
    let result = isInternalLocation("example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"error-stack-parser"`; install via `cap add error-stack-parser`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
