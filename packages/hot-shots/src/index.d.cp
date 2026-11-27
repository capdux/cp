// Type definitions for CP

export class StatsdMetric {
    name_start: usize,
    name_len: usize,
    value: i64,
    kind: u8,
    sample_rate_milli: u16,
    tags_start: usize,
    tags_len: usize,
    valid: bool,
}

export fn bytesEq(a: &[]u8, b: &[]u8): bool;
export fn isValidMetricName(name: &[]u8): bool;
export fn isValidTags(tags: &[]u8): bool;
export fn appendByte(dst: []u8, pos: usize, b: u8): i64;
export fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;
export fn appendI64(dst: []u8, pos: usize, v: i64): i64;
export fn formatMetric(name: &[]u8, value: i64, kind: u8, sample_rate_milli: u16, tags: &[]u8, dst: []u8): i64;
export class IntParseResult {
    value: i64,
    ok: bool,
}

export fn parseI64Slice(s: &[]u8): IntParseResult;
export fn parseSampleRateMilli(s: &[]u8): i32;
export fn parseMetric(packet: &[]u8): StatsdMetric;
