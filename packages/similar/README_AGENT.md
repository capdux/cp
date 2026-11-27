# similar — AI Agent Quick Ref

Zero-allocation line-level LCS diff, unified diff formatter, and similarity ratio engine for CP (clean-room MIT, inspired by Rust similar v2). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DiffOp,
    DiffStats,
    SmdLine,
    smdSplitLines,
    smdLineEq,
    smdEmitOp,
    smdWriteBytes,
    smdWriteStr,
} from "similar";
```

## Core Types & Constants

```cp
class DiffOp { tag: u8, old_idx: usize, new_idx: usize }
class DiffStats { equal_lines: usize, deleted_lines: usize, inserted_lines: usize, total_ops: usize }
class SmdLine { start: usize, end: usize }
```

## API Signatures

```cp
fn smdSplitLines(text: string, out_lines: []SmdLine): usize;
fn smdLineEq(a: string, la: SmdLine, b: string, lb: SmdLine): bool;
fn smdEmitOp(ops_out: []DiffOp, stats: &mut DiffStats, tag: u8, old_idx: usize, new_idx: usize);
fn smdWriteBytes(out: []u8, pos: usize, src: string, start: usize, end: usize): usize;
fn smdWriteStr(out: []u8, pos: usize, s: string): usize;
fn smdWriteByte(out: []u8, pos: usize, b: u8): usize;
fn smdWriteUsize(out: []u8, pos: usize, val: usize): usize;
fn diffLines(old_text: string, new_text: string, ops_out: []DiffOp): DiffStats;
fn hasChanges(old_text: string, new_text: string): bool;
fn similarityRatio(old_text: string, new_text: string): u32;
fn formatUnifiedDiff(old_name: string, new_name: string, old_text: string, new_text: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatUnifiedDiff } from "similar";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatUnifiedDiff("example", "example", "example", "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"similar"`; install via `cap add similar`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
