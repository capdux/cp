// Type definitions for CP

import { normalizePath, hasBackslash } from "../../normalize-path/src/index.cp";
import { isExtglob } from "../../is-extglob/src/index.cp";
export class PicoOptions {
    dot: bool,
    nocase: bool,
    basename: bool,
    noglobstar: bool,
    fn defaults(): PicoOptions;
}

export fn pmLower(ch: u8): u8;
export fn pmCharEq(a: u8, b: u8, nocase: bool): bool;
export fn pmHasSlash(s: string): bool;
export fn pmBasenameSlice(path: string): string;
export fn pmIsLeadingDot(path: string, pIdx: usize): bool;
export fn pmCountLeadingNegations(pat: string): usize;
export fn pmHasHiddenSegment(path: string, pIdx: usize): bool;
export fn pmFindClassEnd(pat: string, gIdx: usize): usize;
export fn pmMatchCharClass(ch: u8, pat: string, gIdx: usize, endIdx: usize, nocase: bool): bool;
export fn pmMatchCore(path: string, pIdx: usize, pat: string, gIdx: usize, opts: PicoOptions, depth: usize): bool;
export fn pmFindBraceGroup(pat: string, outBounds: []usize): bool;
export fn pmMatchWithBraces(path: string, pat: string, opts: PicoOptions, depth: usize): bool;
export fn pmMatchNormalized(normPath: string, pat: string, opts: PicoOptions): bool;
export fn pmSegmentHasGlob(seg: string): bool;
export fn isMatchWithOptions(path: string, pattern: string, opts: PicoOptions): bool;
export fn isMatch(path: string, pattern: string): bool;
export fn isMatchAny(path: string, patterns: &[]string, opts: PicoOptions): bool;
export fn scanBase(pattern: string, out: []u8): usize;
