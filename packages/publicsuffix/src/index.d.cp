// Type definitions for CP

export fn pslToLower(b: u8): u8;
export fn pslIsLabelChar(b: u8): bool;
export fn pslBufEqLowerStr(buf: [256]u8, start: usize, len: usize, s: string): bool;
export fn pslCountPatternLabels(pat: string): usize;
export fn pslMatchSuffixPattern(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize, pat: string): usize;
export fn pslMatchBuiltin3(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool;
export fn pslMatchBuiltin2(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool;
export fn pslMatchBuiltin1(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool;
export class PslRule {
    pattern: string,
    kind: u8,
}

export class DomainInfo {
    norm: [256]u8,
    norm_len: usize,
    suffix_start: usize,
    suffix_len: usize,
    root_start: usize,
    root_len: usize,
    has_root: bool,
    is_known_suffix: bool,
    fn writeHost(out: []u8): usize;
    fn writeSuffix(out: []u8): usize;
    fn writeRegistrable(out: []u8): usize;
    fn suffixEq(s: string): bool;
    fn registrableEq(s: string): bool;
    fn hostEq(s: string): bool;
}

export class PslRuleList {
    rules: [64]PslRule,
    count: usize,
    include_builtin: bool,
    fn new(): PslRuleList;
    fn empty(): PslRuleList;
    fn addRule(_: auto, rule: string): bool;
    fn parseDomain(host: string): DomainInfo?;
    fn suffix(host: string): string?;
    fn registrableDomain(host: string): string?;
    fn isPublicSuffix(host: string): bool;
}

export fn parseDomain(host: string): DomainInfo?;
export fn suffix(host: string): string?;
export fn registrableDomain(host: string): string?;
export fn isPublicSuffix(host: string): bool;
