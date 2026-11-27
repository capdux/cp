// Type definitions for CP

import { strEq, startsWith, endsWith, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class PathRule {
    pattern: string,
    targets: [4]string,
    target_count: usize,
    fn empty(): PathRule;
}

export class PatternMatch {
    matched: bool,
    is_exact: bool,
    prefix_len: usize,
    captured: string,
    fn none(): PatternMatch;
}

export class CandidateList {
    items: [4]string,
    count: usize,
    fn empty(): CandidateList;
    fn itemAt(i: usize): string?;
}

export class PathsMatcher {
    base_url: string,
    rules: [32]PathRule,
    rule_count: usize,
    fn init(base_url: string): PathsMatcher;
    fn addRule(_: auto, pattern: string, target: string);
    fn addRuleMulti(_: auto, pattern: string, t1: string, t2: string);
}

export fn matchesPattern(pattern: string, specifier: string): PatternMatch;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export class SegSpan {
    start: usize,
    end: usize,
}

export fn writeNormalizedPath(raw: string, out: []u8, start_pos: usize): usize;
export fn expandTargetInto(base_url: string, target: string, captured: string, out: []u8, start_pos: usize): usize;
export class BestRuleHit {
    found: bool,
    rule_idx: usize,
    match_info: PatternMatch,
}

export fn findBestRule(matcher: PathsMatcher, specifier: string): BestRuleHit;
export fn resolvePath(matcher: PathsMatcher, specifier: string, out: []u8): string?;
export fn resolveAllCandidates(matcher: PathsMatcher, specifier: string, out_buf: []u8): CandidateList;
