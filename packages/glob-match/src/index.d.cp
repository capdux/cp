// Type definitions for CP

export class GlobSpan {
    start: usize,
    end: usize,
}

export fn gbmFindClassClose(pattern: string, open_idx: usize): usize;
export fn gbmMatchCharClass(pattern: string, open_idx: usize, close_idx: usize, ch: u8): bool;
export fn gbmMatchCore(pattern: string, path: string, record_caps: bool, out_caps: []GlobSpan, out_count: &mut usize): bool;
export fn gbmFindBracePair(pattern: string, open_out: &mut usize, close_out: &mut usize): bool;
export fn gbmCopySlice(dst: []u8, pos: usize, src: string, s: usize, e: usize): usize;
export fn gbmMatchWithBraces(pattern: string, path: string, depth: usize, steps: &mut usize, record_caps: bool, out_caps: []GlobSpan, out_count: &mut usize): bool;
export fn globMatch(pattern: string, path: string): bool;
export fn globMatchAny(patterns: &[]string, path: string): bool;
export fn globMatchCaptures(pattern: string, path: string, out_caps: []GlobSpan): usize;
export fn isStaticGlob(pattern: string): bool;
