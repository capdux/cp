// Type definitions for CP

import { foldAsciiByte } from "../../unicase/src/index.cp";
export class AcMatch {
    pattern_id: usize,
    start: usize,
    end: usize,
}

export fn acdfaNormByte(b: u8, ignore_case: bool): u8;
export fn acdfaPopcount32(mask: u32): usize;
export class AhoCorasick {
    edge_head: [256]u16,
    edge_ch: [256]u8,
    edge_to: [256]u16,
    edge_next: [256]u16,
    fail: [256]u16,
    out_link: [256]u16,
    output_mask: [256]u32,
    pat_len: [32]usize,
    state_count: usize,
    edge_count: usize,
    pat_count: usize,
    ignore_case: bool,
    built: bool,
    fn new(ignore_case: bool): AhoCorasick;
    fn acdfaFindEdge(state: u16, ch: u8): u16;
    fn acdfaStep(state: u16, ch: u8): u16;
    fn addPattern(_: auto, pat: string): bool;
    fn build(_: auto);
    fn patternCount(): usize;
    fn stateCount(): usize;
    fn isMatch(haystack: string): bool;
    fn findFirst(haystack: string): AcMatch?;
    fn findAll(haystack: string, out_matches: []AcMatch): usize;
    fn countMatches(haystack: string): usize;
}

