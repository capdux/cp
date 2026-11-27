// Type definitions for CP

export class ByteRange {
    start: u64,
    end: u64,
}

export class RangeResult {
    status: i32,
    unit: string,
    ranges: [32]ByteRange,
    count: usize,
}

export fn rpIsWs(b: u8): bool;
export fn rpTrim(s: string): string;
export fn rpParseU64(s: string, ok_out: []bool): u64;
export fn parseRange(size: u64, header: string, combine: bool): RangeResult;
export fn rangeByteLength(r: ByteRange): u64;
