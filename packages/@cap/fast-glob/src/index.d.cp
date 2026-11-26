// Type definitions for CP

import { globMatch, isStaticGlob } from "../../glob-match/src/index.cp";
export fn fglToLower(b: u8): u8;
export fn fglIsGlobMeta(b: u8): bool;
export fn fglNormalizeRelPath(raw: string): string;
export fn fglPathDepth(path: string): usize;
export fn fglHasDotSegment(path: string): bool;
export fn fglLowerInto(src: string, dst: []u8): usize;
export fn fglMatchOne(pattern: string, path: string, case_sensitive: bool): bool;
export class FastGlobOptions {
    dot: bool,
    deep: usize,
    only_files: bool,
    only_directories: bool,
    case_sensitive: bool,
    fn defaults(): FastGlobOptions;
}

export fn isDynamicPattern(pattern: string): bool;
export fn escapePath(path: string, out: []u8): usize;
export fn convertPathToPattern(path: string, out: []u8): usize;
export class GlobPatternSet {
    opts: FastGlobOptions,
    includes: [16]string,
    include_count: usize,
    ignores: [16]string,
    ignore_count: usize,
    fn new(opts: FastGlobOptions): GlobPatternSet;
    fn addPattern(_: auto, raw: string): bool;
    fn addIgnore(_: auto, pat: string): bool;
    fn matches(rel_path: string, is_dir: bool): bool;
}

