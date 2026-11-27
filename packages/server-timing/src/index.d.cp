// Type definitions for CP

export class TimingMetric {
    name: string,
    has_dur: bool,
    dur_ms: f64,
    desc: string,
}

export class ParsedFloat {
    val: f64,
    ok: bool,
}

export fn isSpace(b: u8): bool;
export fn toLowerAscii(b: u8): u8;
export fn strEq(a: string, b: string): bool;
export fn eqIgnoreCase(a: string, lower_b: string): bool;
export fn trimSlice(s: string): string;
export fn parseFloatSimple(s: string): ParsedFloat;
export fn parseSingleMetric(seg: string, out: &mut TimingMetric): bool;
export fn parseHeader(header: string, out: []TimingMetric): usize;
export fn findMetric(header: string, name: string, out: &mut TimingMetric): bool;
export fn totalDurationMs(header: string): f64;
export fn isTokenChar(b: u8): bool;
export fn descNeedsQuotes(s: string): bool;
export fn pushSlice(dst: []u8, pos: usize, src: string): i64;
export fn pushU64(dst: []u8, pos: usize, val: u64): i64;
export fn pushDurMs(dst: []u8, pos: usize, dur_ms: f64): i64;
export fn formatMetric(metric: TimingMetric, out: []u8): usize;
export fn formatHeader(metrics: &[]TimingMetric, out: []u8): usize;
