// Type definitions for CP

import { MinimatchOptions, minimatchWithOptions } from "../../minimatch/src/index.cp";
import { normalizePath } from "../../normalize-path/src/index.cp";
export class IgnoreRule {
    pat: [128]u8,
    pat_len: usize,
    negated: bool,
    dir_only: bool,
    anchored: bool,
}

export fn igTrimRightSpaces(s: string): usize;
export fn igHasInnerSlash(s: string, start: usize, end: usize): bool;
export fn igRuleMatches(rule: IgnoreRule, rel_path: string, is_dir: bool, case_insensitive: bool): bool;
export fn igMatchSingle(pat: &[]u8, anchored: bool, path: string, case_insensitive: bool): bool;
export class IgnoreMatcher {
    rules: [64]IgnoreRule,
    count: usize,
    ignore_case: bool,
    fn new(): IgnoreMatcher;
    fn newCaseInsensitive(): IgnoreMatcher;
    fn ruleCount(): usize;
    fn addRule(_: auto, raw_line: string): bool;
    fn addLines(_: auto, content: string): usize;
    fn ignores(path: string): bool;
    fn filter(paths: &[]string, out_indices: []usize): usize;
}

