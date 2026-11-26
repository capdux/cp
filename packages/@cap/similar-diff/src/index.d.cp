// Type definitions for CP

export class DiffOp {
    tag: u8,
    old_idx: usize,
    new_idx: usize,
}

export class DiffStats {
    equal_lines: usize,
    deleted_lines: usize,
    inserted_lines: usize,
    total_ops: usize,
}

export class SmdLine {
    start: usize,
    end: usize,
}

export fn smdSplitLines(text: string, out_lines: []SmdLine): usize;
export fn smdLineEq(a: string, la: SmdLine, b: string, lb: SmdLine): bool;
export fn smdEmitOp(ops_out: []DiffOp, stats: &mut DiffStats, tag: u8, old_idx: usize, new_idx: usize);
export fn smdWriteBytes(out: []u8, pos: usize, src: string, start: usize, end: usize): usize;
export fn smdWriteStr(out: []u8, pos: usize, s: string): usize;
export fn smdWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn smdWriteUsize(out: []u8, pos: usize, val: usize): usize;
export fn diffLines(old_text: string, new_text: string, ops_out: []DiffOp): DiffStats;
export fn hasChanges(old_text: string, new_text: string): bool;
export fn similarityRatio(old_text: string, new_text: string): u32;
export fn formatUnifiedDiff(old_name: string, new_name: string, old_text: string, new_text: string, out: []u8): usize;
