// Type definitions for CP

import { FastGlobOptions, GlobPatternSet } from "../../fast-glob/src/index.cp";
export class RimrafOptions {
    preserve_root: bool,
    max_retries: u8,
    retry_delay_ms: u32,
    is_windows: bool,
    fn defaults(): RimrafOptions;
}

export class RimrafPlan {
    valid: bool,
    paths: [32]string,
    count: usize,
}

export fn rmfIsAsciiAlpha(b: u8): bool;
export fn rmfIsSep(b: u8, is_windows: bool): bool;
export fn rmfHasNul(s: string): bool;
export fn rmfPathDepth(path: string, is_windows: bool): usize;
export fn isRootPath(path: string, is_windows: bool): bool;
export fn validateRimrafTarget(target: string, opts: RimrafOptions): bool;
export fn rmfSortByDepthDesc(paths: []string, count: usize, is_windows: bool);
export fn planBottomUpDelete(entries: []string, opts: RimrafOptions): RimrafPlan;
export fn planFilteredDelete(entries: []string, pattern: string, opts: RimrafOptions): RimrafPlan;
