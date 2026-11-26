// Type definitions for CP

export fn pdurCheckedAddU64(a: u64, b: u64): u64?;
export fn pdurCheckedMulU64(a: u64, b: u64): u64?;
export fn pdurIsSpace(c: u8): bool;
export fn pdurIsDigit(c: u8): bool;
export fn pdurIsAlpha(c: u8): bool;
export fn pdurToLower(c: u8): u8;
export fn pdurBufEq(buf: &[]u8, len: usize, lit: string): bool;
export fn pdurMatchUnit(buf: &[]u8, len: usize): u32;
export fn pdurWriteBytes(out: []u8, pos: usize, s: string): usize;
export fn pdurWriteU64(out: []u8, pos: usize, val: u64): usize;
export fn pdurAppendPart(out: []u8, pos: usize, val: u64, suffix: string): usize;
export class ParsedDuration {
    secs: u64,
    nanos: u32,
    fn isZero(): bool;
    fn totalSecs(): u64;
    fn totalMillis(): u64;
    fn totalNanos(): u64?;
}

export fn parseDuration(input: string): ParsedDuration?;
export fn formatDuration(d: ParsedDuration, out: []u8): usize;
