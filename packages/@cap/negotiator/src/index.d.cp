// Type definitions for CP

export class NegSpec {
    start: usize,
    len: usize,
    q_milli: u16,
    specificity: u8,
    order: usize,
}

export fn isSpace(ch: u8): bool;
export fn toLower(ch: u8): u8;
export fn eqIgnoreCase(a: string, b: string): bool;
export fn findByte(s: string, ch: u8): i64;
export fn trimStart(s: string, lo: usize, hi: usize): usize;
export fn trimEnd(s: string, lo: usize, hi: usize): usize;
export fn parseQMilli(val: string): u16;
export fn classifyToken(tok: string): u8;
export fn parseAcceptList(header: string, out: []NegSpec): usize;
export fn isBetterCandidate(q: u16, spec: u8, order: usize, best_q: u16, best_spec: u8, best_order: usize, has_best: bool): bool;
export fn bestEncoding(header: string, available: &[]string): i64;
export fn bestMediaType(header: string, available: &[]string): i64;
export fn langPrefixMatch(longer: string, shorter: string): bool;
export fn bestLanguage(header: string, available: &[]string): i64;
