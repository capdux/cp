# clean-stack — AI Agent Quick Ref

Zero-allocation stack trace cleaner removing internal runtime frames and normalizing base paths (clean-room MIT implementation of npm clean-stack v5) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CleanStackOptions,
    csContains,
    csStartsWith,
    csAppend,
    isInternalStackFrame,
    csWriteCleanedLine,
    cleanStackWithOptions,
    cleanStack,
} from "clean-stack";
```

## Core Types & Constants

```cp
class CleanStackOptions { pretty: bool, base_path: string, home_dir: string } // methods: defaults(...)
```

## API Signatures

```cp
fn csContains(hay: string, needle: string): bool;
fn csStartsWith(s: string, prefix: string): bool;
fn csAppend(out: []u8, pos: usize, s: string): usize;
fn isInternalStackFrame(line: string): bool;
fn csWriteCleanedLine(line: string, opts: CleanStackOptions, out: []u8, pos: usize): usize;
fn cleanStackWithOptions(stack: string, opts: CleanStackOptions, out: []u8): usize;
fn cleanStack(stack: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isInternalStackFrame } from "clean-stack";

pub fn main() {
    let result = isInternalStackFrame("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"clean-stack"`; install via `cap add clean-stack`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
