# debug — AI Agent Quick Ref

Zero-allocation namespace debug log filter, glob exclusion engine, deterministic ANSI coloring, and diff-ms formatter for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DebugFilter,
    DebugLogger,
    isSep,
    globMatch,
    appendByte,
    appendStr,
    appendU64,
    isNamespaceEnabled,
} from "debug";
```

## Core Types & Constants

```cp
class DebugFilter { raw_pattern: string } // methods: from(...) +1
class DebugLogger { namespace: string, color_code: u8, last_ms: i64, enabled: bool } // methods: create(...) +1
```

## API Signatures

```cp
fn isSep(c: u8): bool;
fn globMatch(pat: string, text: string): bool;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn appendStr(out: []u8, pos: usize, s: string): usize;
fn appendU64(out: []u8, pos: usize, val: u64): usize;
fn isNamespaceEnabled(pattern: string, namespace: string): bool;
fn selectColor(namespace: string): u8;
fn formatDiffMs(diff_ms: i64, out: []u8): string;
fn formatDebugLine(ns: string, msg: string, diff_ms: i64, use_color: bool, out: []u8): string;
```

## Minimal Usage

```cp
import { isNamespaceEnabled } from "debug";

pub fn main() {
    let result = isNamespaceEnabled("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"debug"`; install via `cap add debug`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
