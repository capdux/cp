// Type definitions for CP

export fn pdStrEq(a: string, b: string): bool;
export fn pdPush(out: []u8, pos: usize, s: string): usize;
export class SegList {
    segs: [32]string,
    count: usize,
    is_abs: bool,
}

export fn pdSplitNormalized(path: string): SegList;
export fn diffPaths(target: string, base: string, out: []u8): usize?;
