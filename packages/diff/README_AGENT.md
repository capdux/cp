# diff — AI Agent Quick Ref

Zero-allocation Myers O(ND) line & character diff engine with FNV-1a hash pre-filtering, GNU Unified Diff formatting, and lossless patch application for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DiffOp,
    DiffError,
    DiffEdit,
    DiffStats,
    LineDiffResult,
    LineEntry,
    BufWriter,
    fnv1a64,
} from "diff";
```

## Core Types & Constants

```cp
error DiffError { TooManyLines, BufferTooSmall, PatchFailed, InvalidPatch }
enum DiffOp { Equal, Delete, Insert }
class DiffEdit { op: DiffOp, old_index: usize, new_index: usize, text: &[]u8 }
class DiffStats { equal: usize, inserted: usize, deleted: usize } // methods: similarity(...)
class LineDiffResult { edits: [MAX_EDITS]DiffEdit, len: usize } // methods: stats(...) +3
class LineEntry { text: &[]u8, hash: u64 }
class BufWriter { buf: []u8, pos: usize } // methods: writeByte(...) +3
```

## API Signatures

```cp
fn fnv1a64(data: &[]u8): u64;
fn linesEqual(a: LineEntry, b: LineEntry): bool;
fn splitLines(text: &[]u8, out: []LineEntry): usize ! DiffError;
fn traceSlot(d: usize, k: i32): usize;
fn myersDiffEntries(old_entries: &[]LineEntry, new_entries: &[]LineEntry): LineDiffResult ! DiffError;
fn diffLines(old_text: &[]u8, new_text: &[]u8): LineDiffResult ! DiffError;
fn diffChars(old_text: &[]u8, new_text: &[]u8): LineDiffResult ! DiffError;
fn formatUnifiedDiff(old_name: &[]u8, new_name: &[]u8, old_text: &[]u8, new_text: &[]u8, context_lines: usize, out: []u8): &[]u8 ! DiffError;
fn applyLineDiff(old_text: &[]u8, diff_res: LineDiffResult, out: []u8): &[]u8 ! DiffError;
```

## Minimal Usage

```cp
import { formatUnifiedDiff } from "diff";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let mut buf_2: [256]u8 = undefined;
    let mut buf_3: [256]u8 = undefined;
    let mut buf_4: [256]u8 = undefined;
    let result = formatUnifiedDiff(out_buf[0..256], buf_1[0..256], buf_2[0..256], buf_3[0..256], 0, buf_4[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"diff"`; install via `cap add diff`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
