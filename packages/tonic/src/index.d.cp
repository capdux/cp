// Type definitions for CP

import { trim } from "cap:ascii";
import { readU32BE, writeU32BE } from "cap:bin";
import { parseU64, writeStr, writeU64 } from "cap:fmt";
export class GrpcFrameHeader {
    compressed: bool,
    payload_len: u32,
    total_len: usize,
}

export fn statusName(code: u8): string;
export fn isRetryableStatus(code: u8): bool;
export fn encodeFrame(compressed: bool, payload: &[]u8, out: []u8): usize;
export fn decodeHeader(data: &[]u8, out: &mut GrpcFrameHeader): i32;
export fn parseTimeoutMicros(header: string): i64;
export fn formatTimeoutMillis(ms: u64, out: []u8): usize;
