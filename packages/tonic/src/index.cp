// @cap/grpc-frame — Zero-allocation gRPC over HTTP/2 Framing, Status Codes & Timeout Utilities
// Clean-room MIT implementation in pure CP.

import { trim } from "cap:ascii";
import { readU32BE, writeU32BE } from "cap:bin";
import { parseU64, writeStr, writeU64 } from "cap:fmt";

// Canonical gRPC status codes (0..16)
pub const STATUS_OK: u8 = 0;
pub const STATUS_CANCELLED: u8 = 1;
pub const STATUS_UNKNOWN: u8 = 2;
pub const STATUS_INVALID_ARGUMENT: u8 = 3;
pub const STATUS_DEADLINE_EXCEEDED: u8 = 4;
pub const STATUS_NOT_FOUND: u8 = 5;
pub const STATUS_ALREADY_EXISTS: u8 = 6;
pub const STATUS_PERMISSION_DENIED: u8 = 7;
pub const STATUS_RESOURCE_EXHAUSTED: u8 = 8;
pub const STATUS_FAILED_PRECONDITION: u8 = 9;
pub const STATUS_ABORTED: u8 = 10;
pub const STATUS_OUT_OF_RANGE: u8 = 11;
pub const STATUS_UNIMPLEMENTED: u8 = 12;
pub const STATUS_INTERNAL: u8 = 13;
pub const STATUS_UNAVAILABLE: u8 = 14;
pub const STATUS_DATA_LOSS: u8 = 15;
pub const STATUS_UNAUTHENTICATED: u8 = 16;

// Frame decode result codes
pub const GRPC_OK: i32 = 0;
pub const GRPC_INCOMPLETE: i32 = 1;
pub const GRPC_INVALID: i32 = -1;

pub struct GrpcFrameHeader {
    pub compressed: bool,
    pub payload_len: u32,
    pub total_len: usize,
}

/// Returns the canonical gRPC status name for codes `0..16`, or `""` for unknown codes.
pub fn statusName(code: u8) -> string {
    if (code == 0) { return "OK"; }
    if (code == 1) { return "CANCELLED"; }
    if (code == 2) { return "UNKNOWN"; }
    if (code == 3) { return "INVALID_ARGUMENT"; }
    if (code == 4) { return "DEADLINE_EXCEEDED"; }
    if (code == 5) { return "NOT_FOUND"; }
    if (code == 6) { return "ALREADY_EXISTS"; }
    if (code == 7) { return "PERMISSION_DENIED"; }
    if (code == 8) { return "RESOURCE_EXHAUSTED"; }
    if (code == 9) { return "FAILED_PRECONDITION"; }
    if (code == 10) { return "ABORTED"; }
    if (code == 11) { return "OUT_OF_RANGE"; }
    if (code == 12) { return "UNIMPLEMENTED"; }
    if (code == 13) { return "INTERNAL"; }
    if (code == 14) { return "UNAVAILABLE"; }
    if (code == 15) { return "DATA_LOSS"; }
    if (code == 16) { return "UNAUTHENTICATED"; }
    return "";
}

/// Returns true if the gRPC status code is conventionally safe to retry
/// (`UNAVAILABLE`, `RESOURCE_EXHAUSTED`, or `ABORTED`).
pub fn isRetryableStatus(code: u8) -> bool {
    return code == STATUS_UNAVAILABLE
        || code == STATUS_RESOURCE_EXHAUSTED
        || code == STATUS_ABORTED;
}

/// Encodes a gRPC Length-Prefixed Message: 1-byte compressed flag + 4-byte big-endian length + payload.
/// Returns total bytes written (`5 + payload.len`), or `0` if `out` is too small.
pub fn encodeFrame(compressed: bool, payload: []const u8, out: []u8) -> usize {
    const total: usize = payload.len + 5;
    if (total > out.len) {
        return 0;
    }
    out[0] = if (compressed) 1 else 0;
    * = writeU32BE(out, 1, payload.len as u32);
    return writeStr(payload, out, 5);
}

/// Decodes a 5-byte gRPC Length-Prefixed Message header into `out`.
/// Returns `GRPC_OK (0)` when the full frame is available in `data`,
/// `GRPC_INCOMPLETE (1)` when more bytes are needed, or `GRPC_INVALID (-1)` on illegal compression flag.
pub fn decodeHeader(data: []const u8, out: *GrpcFrameHeader) -> i32 {
    if (data.len < 5) {
        return GRPC_INCOMPLETE;
    }
    const flag = data[0];
    if (flag != 0 && flag != 1) {
        return GRPC_INVALID;
    }
    const len_be: u32 = readU32BE(data, 1) ?? 0;
    const total: usize = (len_be as usize) + 5;

    out.compressed = (flag == 1);
    out.payload_len = len_be;
    out.total_len = total;

    if (data.len < total) {
        return GRPC_INCOMPLETE;
    }
    return GRPC_OK;
}

/// Parses a `grpc-timeout` header (1..8 ASCII digits + unit `H`/`M`/`S`/`m`/`u`/`n`)
/// into microseconds. Returns `-1` on invalid syntax or unit.
pub fn parseTimeoutMicros(header: string) -> i64 {
    const trimmed = trim(header);
    const slen = trimmed.len;
    if (slen < 2 || slen > 9 || trimmed[0] == '+') {
        return -1;
    }

    const unit = trimmed[slen - 1];
    let val: u64 = 0;
    if (parsed := parseU64(trimmed[0..(slen - 1)])) {
        val = parsed;
    } else {
        return -1;
    }

    if (unit == 72) {
        // 'H' hours
        return (val * 3600000000) as i64;
    }
    if (unit == 77) {
        // 'M' minutes
        return (val * 60000000) as i64;
    }
    if (unit == 83) {
        // 'S' seconds
        return (val * 1000000) as i64;
    }
    if (unit == 109) {
        // 'm' milliseconds
        return (val * 1000) as i64;
    }
    if (unit == 117) {
        // 'u' microseconds
        return val as i64;
    }
    if (unit == 110) {
        // 'n' nanoseconds
        return (val / 1000) as i64;
    }
    return -1;
}

/// Formats a timeout in milliseconds into a valid `grpc-timeout` header (`<val>m`, `<val>S`, or `<val>M`)
/// respecting gRPC's 8-digit value limit. Returns bytes written, or `0` if `out` is too small.
pub fn formatTimeoutMillis(ms: u64, out: []u8) -> usize {
    let val = ms;
    let unit: u8 = 109; // 'm'
    if (val > 99999999) {
        val = val / 1000;
        unit = 83; // 'S'
    }
    if (val > 99999999) {
        val = val / 60;
        unit = 77; // 'M'
    }
    if (val > 99999999) {
        val = val / 60;
        unit = 72; // 'H'
    }

    let tmp: [16]u8 = undefined;
    const n = writeU64(val, tmp[0..16], 0);
    if (n + 1 > out.len) {
        return 0;
    }
    const w = writeStr(tmp[0..n], out, 0);
    out[w] = unit;
    return w + 1;
}
