# diff@1.0.0 — AI Agent Quick Ref

- **Import**: `import { DiffOp, DiffError, diffLines, diffChars, formatUnifiedDiff, applyLineDiff } from "diff";`
- **Provenance**: npm `diff` (`jsdiff` v7) + `fast-diff` parity | Myers O(ND) + FNV-1a | Zero-heap | License: MIT
- **Constants**: `MAX_LINES = 256` (max lines per side / max bytes for `diffChars`), `MAX_EDITS = 512`
- **Types**: `src/index.d.cp`

## Core Types & Functions
```cp
export error DiffError { TooManyLines, BufferTooSmall, PatchFailed, InvalidPatch }
export enum DiffOp { Equal = 0, Delete = 1, Insert = 2 }
export class DiffEdit { op: DiffOp, old_index: usize, new_index: usize, text: &[]u8 }
export class DiffStats { equal: usize, inserted: usize, deleted: usize, fn similarity(): f64; }
export class LineDiffResult {
    edits: [MAX_EDITS]DiffEdit, len: usize,
    fn stats(): DiffStats;
    fn isIdentical(): bool;
    fn slice(): &[]DiffEdit;
    fn get(idx: usize): DiffEdit?;
}

export fn diffLines(old_text: &[]u8, new_text: &[]u8): LineDiffResult ! DiffError;
export fn diffChars(old_text: &[]u8, new_text: &[]u8): LineDiffResult ! DiffError;
export fn formatUnifiedDiff(old_name: &[]u8, new_name: &[]u8, old_text: &[]u8, new_text: &[]u8, context_lines: usize, out: []u8): &[]u8 ! DiffError;
export fn applyLineDiff(old_text: &[]u8, diff_res: LineDiffResult, out: []u8): &[]u8 ! DiffError;
```

## Minimal Example
```cp
import { diffLines, formatUnifiedDiff, applyLineDiff } from "diff";

const res = diffLines("a\nb\n", "a\nB\n") catch return;
let patch_buf: [512]u8 = undefined;
const patch = formatUnifiedDiff("a", "b", "a\nb\n", "a\nB\n", 1, patch_buf[0..]) catch return;
let out: [128]u8 = undefined;
const rebuilt = applyLineDiff("a\nb\n", res, out[0..]) catch return; // "a\nB\n"
```

## Gotchas
1. Stack-bounded limits: each side supports up to `MAX_LINES = 256` lines (`diffLines`) or 256 bytes (`diffChars`); exceeding returns `DiffError.TooManyLines`.
2. `DiffEdit.text` slices borrow directly from `old_text` / `new_text` — keep input slices alive while reading `LineDiffResult`.
3. `applyLineDiff` verifies every `Equal`/`Delete` segment against `old_text` and returns `DiffError.PatchFailed` on mismatch, or `DiffError.BufferTooSmall` if `out` cannot hold the result.
4. `formatUnifiedDiff` automatically merges overlapping/adjacent hunks and appends `\ No newline at end of file` when needed.
5. `DiffStats.similarity()` returns `f64` in `[0.0, 1.0]` (`1.0` when both inputs are empty or identical).
