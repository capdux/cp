// Type definitions for CP

export class MsResult {
    ms: i64,
    valid: bool,
}

export fn mpToLower(c: u8): u8;
export fn mpIsSpace(c: u8): bool;
export fn mpTrim(s: &[]u8): &[]u8;
export fn mpEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn mpUnitMultiplier(unit: &[]u8): f64;
export fn parseMs(input: string): MsResult;
export fn mpWriteRaw(out: []u8, pos: &mut usize, src: &[]u8): bool;
export fn mpWriteU64(out: []u8, pos: &mut usize, val: u64): bool;
export fn mpAbsU64(ms: i64): u64;
export fn mpFormatImpl(ms: i64, longFmt: bool, out: []u8): usize;
export fn formatShort(ms: i64, out: []u8): usize;
export fn formatLong(ms: i64, out: []u8): usize;
