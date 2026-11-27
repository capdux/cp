// Type definitions for CP

import { trim } from "cap:ascii";
import { parseU64 } from "cap:fmt";
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

export fn parseRange(size: u64, header: string, combine: bool): RangeResult;
export fn rangeByteLength(r: ByteRange): u64;
