// Type definitions for CP

export error DiffError { TooManyLines, BufferTooSmall, PatchFailed, InvalidPatch }

export enum DiffOp {
    Equal = 0,
    Delete = 1,
    Insert = 2
}

export class DiffEdit {
    op: DiffOp,
    old_index: usize,
    new_index: usize,
    text: &[]u8,
}

export class DiffStats {
    equal: usize,
    inserted: usize,
    deleted: usize,
    fn similarity(): f64;
}

export class LineDiffResult {
    edits: [MAX_EDITS]DiffEdit,
    len: usize,
    fn stats(): DiffStats;
    fn isIdentical(): bool;
    fn slice(): &[]DiffEdit;
    fn get(idx: usize): DiffEdit?;
}

export class LineEntry {
    text: &[]u8,
    hash: u64,
}

export fn fnv1a64(data: &[]u8): u64;
export fn linesEqual(a: LineEntry, b: LineEntry): bool;
export fn splitLines(text: &[]u8, out: []LineEntry): usize ! DiffError;
export fn traceSlot(d: usize, k: i32): usize;
export fn myersDiffEntries(old_entries: &[]LineEntry, new_entries: &[]LineEntry): LineDiffResult ! DiffError;
export fn diffLines(old_text: &[]u8, new_text: &[]u8): LineDiffResult ! DiffError;
export fn diffChars(old_text: &[]u8, new_text: &[]u8): LineDiffResult ! DiffError;
export class BufWriter {
    buf: []u8,
    pos: usize,
    fn writeByte(_: auto, b: u8): void ! DiffError;
    fn writeSlice(_: auto, s: &[]u8): void ! DiffError;
    fn writeUsize(_: auto, val: usize): void ! DiffError;
    fn written(): &[]u8;
}

export fn formatUnifiedDiff(old_name: &[]u8, new_name: &[]u8, old_text: &[]u8, new_text: &[]u8, context_lines: usize, out: []u8): &[]u8 ! DiffError;
export fn applyLineDiff(old_text: &[]u8, diff_res: LineDiffResult, out: []u8): &[]u8 ! DiffError;
