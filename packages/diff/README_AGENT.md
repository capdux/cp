# diff@1.0.0 (Agent Quickref)

> Zero-allocation Myers O(ND) line & character diff engine with FNV-1a hash pre-filtering, GNU Unified Diff formatting, and lossless patch application for CP | License: MIT
> Import: `import { ... } from "diff";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum DiffOp`

## Functions
- `export fn fnv1a64(data: &[]u8): u64;`
- `export fn linesEqual(a: LineEntry, b: LineEntry): bool;`
- `export fn splitLines(text: &[]u8, out: []LineEntry): usize ! DiffError;`
- `export fn traceSlot(d: usize, k: i32): usize;`
- `export fn myersDiffEntries(old_entries: &[]LineEntry, new_entries: &[]LineEntry): LineDiffResult ! DiffError;`
- `export fn diffLines(old_text: &[]u8, new_text: &[]u8): LineDiffResult ! DiffError;`
- `export fn diffChars(old_text: &[]u8, new_text: &[]u8): LineDiffResult ! DiffError;`
- `export fn formatUnifiedDiff(old_name: &[]u8, new_name: &[]u8, old_text: &[]u8, new_text: &[]u8, context_lines: usize, out: []u8): &[]u8 ! DiffError;`
- `export fn applyLineDiff(old_text: &[]u8, diff_res: LineDiffResult, out: []u8): &[]u8 ! DiffError;`
