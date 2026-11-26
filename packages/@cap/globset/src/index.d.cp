// Type definitions for CP

import { PicoOptions, isMatchWithOptions, scanBase } from "../../picomatch/src/index.cp";
import { normalizePath } from "../../normalize-path/src/index.cp";
export class GlobSetOptions {
    case_insensitive: bool,
    literal_separator: bool,
    backslash_escape: bool,
    fn defaults(): GlobSetOptions;
}

export fn gsToLower(b: u8): u8;
export fn gsByteEq(a: u8, b: u8, nocase: bool): bool;
export fn gsHasPrefix(path: string, prefix: &[]u8, nocase: bool): bool;
export fn gsToPicoOptions(opts: GlobSetOptions): PicoOptions;
export fn gsMatchOne(normPath: string, pat: string, opts: GlobSetOptions, picoOpts: PicoOptions): bool;
export class GlobSet {
    patterns: [32]string,
    base_buf: [32][64]u8,
    base_lens: [32]usize,
    count: usize,
    opts: GlobSetOptions,
    fn new(): GlobSet;
    fn withOptions(opts: GlobSetOptions): GlobSet;
    fn add(_: auto, pattern: string): bool;
    fn len(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn patternAt(idx: usize): string;
    fn isMatch(path: string): bool;
    fn firstMatch(path: string): i32;
    fn matches(path: string, out_indices: []usize): usize;
    fn matchCount(path: string): usize;
}

export class GlobOverride {
    set: GlobSet,
    negated: [32]bool,
    has_whitelist: bool,
    fn new(): GlobOverride;
    fn withOptions(opts: GlobSetOptions): GlobOverride;
    fn add(_: auto, rule: string): bool;
    fn len(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn matched(path: string): i32;
    fn isAllowed(path: string): bool;
}

