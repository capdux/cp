// Type definitions for CP

import { globMatch, isStaticGlob } from "../../glob-match/src/index.cp";
export class ScanResult {
    prefix: string,
    input: string,
    base: string,
    glob: string,
    is_glob: bool,
    negated: bool,
}

export class PicomatchOptions {
    dot: bool,
    nocase: bool,
    match_base: bool,
    bash: bool,
    fn defaults(): PicomatchOptions;
}

export fn pmIsExtglobOp(ch: u8): bool;
export fn pmContainsSlash(s: string): bool;
export fn pmCopySlice(dst: []u8, pos: usize, src: string, start: usize, end: usize): usize;
export fn pmToLowerAscii(dst: []u8, src: string): usize;
export fn pmFindClassClose(s: string, open_idx: usize): usize;
export fn pmFindBraceClose(s: string, open_idx: usize): usize;
export fn pmFindParenClose(s: string, open_paren: usize): usize;
export fn pmFindExtglob(pat: string, start_out: &mut usize, end_out: &mut usize): bool;
export fn scan(pattern: string): ScanResult;
export fn pmMatchAnyBranch(text: string, inner: string, depth: usize, steps: &mut usize): bool;
export fn pmMatchRepeat(text: string, inner: string, depth: usize, steps: &mut usize): bool;
export fn pmMatchExtglobInner(text: string, pat: string, depth: usize, steps: &mut usize): bool;
export fn matchExtglob(text: string, pat: string): bool;
export fn pmBasename(path: string): string;
export fn pmHasDotSegment(path: string): bool;
export fn pmPatternAllowsDot(pat: string): bool;
export fn isMatch(path: string, pattern: string, opts: PicomatchOptions): bool;
