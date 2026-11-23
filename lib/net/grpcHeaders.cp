// Reference-hardened against hyperium/tonic (MIT)
// Source: cp/lib/grpc_wire/headers.cp (grpc-timeout, percent-encoded grpc-message, -bin base64 headers & 16-lane SIMD metadata precheck)

const std = @import("std");

type GrpcHdrVec16u8 = @Vector(16, u8);

const MAX_TIMEOUT_DIGITS: usize = 8;
const MAX_TIMEOUT_VALUE: u64 = 99_999_999;

fn grpcHdrLoadVec16(data: []const u8, off: usize): GrpcHdrVec16u8 {
    const v: GrpcHdrVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD precheck for valid gRPC ASCII metadata values (`0x20..=0x7E`).
pub fn simdGrpcIsValidAsciiMetadata(val: []const u8): bool {
    let i: usize = 0;
    const n = val.len;
    const v_lo: GrpcHdrVec16u8 = @splat(0x20);
    const v_hi: GrpcHdrVec16u8 = @splat(0x7E);
    while (i + 16 <= n) {
        const chunk = grpcHdrLoadVec16(val, i);
        const ok_mask: u16 = @bitCast((chunk >= v_lo) & (chunk <= v_hi));
        if (ok_mask != 0xFFFF) return false;
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (val[i] < 0x20 || val[i] > 0x7E) return false;
    }
    return true;
}

/// Scalar reference for `simdGrpcIsValidAsciiMetadata` (differential verification).
pub fn simdGrpcIsValidAsciiMetadataScalar(val: []const u8): bool {
    let i: usize = 0;
    while (i < val.len) : (i += 1) {
        if (val[i] < 0x20 || val[i] > 0x7E) return false;
    }
    return true;
}

/// 16-lane `@Vector(16, u8)` SIMD scan checking if `msg` requires percent-encoding
/// (contains `< 0x20`, `> 0x7E`, or `'%'`).
pub fn simdGrpcNeedsPercentEncode(msg: []const u8): bool {
    let i: usize = 0;
    const n = msg.len;
    const v_lo: GrpcHdrVec16u8 = @splat(0x20);
    const v_hi: GrpcHdrVec16u8 = @splat(0x7E);
    const v_pct: GrpcHdrVec16u8 = @splat('%');
    while (i + 16 <= n) {
        const chunk = grpcHdrLoadVec16(msg, i);
        const ok_range: u16 = @bitCast((chunk >= v_lo) & (chunk <= v_hi));
        const has_pct: u16 = @bitCast(chunk == v_pct);
        if (ok_range != 0xFFFF || has_pct != 0) return true;
        i += 16;
    }
    while (i < n) : (i += 1) {
        const b = msg[i];
        if (b < 0x20 || b > 0x7E || b == '%') return true;
    }
    return false;
}

/// Scalar reference for `simdGrpcNeedsPercentEncode` (differential verification).
pub fn simdGrpcNeedsPercentEncodeScalar(msg: []const u8): bool {
    let i: usize = 0;
    while (i < msg.len) : (i += 1) {
        const b = msg[i];
        if (b < 0x20 || b > 0x7E || b == '%') return true;
    }
    return false;
}

/// Returns `true` if `key` is a valid gRPC binary metadata key (non-empty prefix + `"-bin"` suffix).
pub fn isBinHeaderKey(key: []const u8): bool {
    if (key.len <= 4) return false;
    const off = key.len - 4;
    return key[off] == '-' && key[off + 1] == 'b' && key[off + 2] == 'i' && key[off + 3] == 'n';
}

pub fn parseGrpcTimeoutNs(header_val: []const u8): !u64 {
    if (header_val.len < 2) {
        return error.InvalidTimeout;
    }
    const num_part = header_val[0 .. header_val.len - 1];
    const unit = header_val[header_val.len - 1];

    if (num_part.len == 0 or num_part.len > MAX_TIMEOUT_DIGITS) {
        return error.InvalidTimeout;
    }

    let value: u64 = 0;
    let i: usize = 0;
    while (i < num_part.len) {
        const ch = num_part[i];
        if (ch < '0' or ch > '9') {
            return error.InvalidTimeout;
        }
        value = value * 10 + @as(u64, ch - '0');
        i += 1;
    }

    const multiplier: u64 = switch (unit) {
        'H' => 3_600_000_000_000,
        'M' => 60_000_000_000,
        'S' => 1_000_000_000,
        'm' => 1_000_000,
        'u' => 1_000,
        'n' => 1,
        else => return error.InvalidTimeout,
    };

    const res = std.math.mul(u64, value, multiplier) catch {
        return error.TimeoutOverflow;
    };
    return res;
}

pub fn formatGrpcTimeout(alloc: std.mem.Allocator, ns: u64): ![]u8 {
    if (ns <= MAX_TIMEOUT_VALUE) {
        return std.fmt.allocPrint(alloc, "{d}n", .{ns});
    }
    const us = ns / 1_000;
    if (us <= MAX_TIMEOUT_VALUE) {
        return std.fmt.allocPrint(alloc, "{d}u", .{us});
    }
    const ms = ns / 1_000_000;
    if (ms <= MAX_TIMEOUT_VALUE) {
        return std.fmt.allocPrint(alloc, "{d}m", .{ms});
    }
    const secs = ns / 1_000_000_000;
    if (secs <= MAX_TIMEOUT_VALUE) {
        return std.fmt.allocPrint(alloc, "{d}S", .{secs});
    }
    const mins = ns / 60_000_000_000;
    if (mins <= MAX_TIMEOUT_VALUE) {
        return std.fmt.allocPrint(alloc, "{d}M", .{mins});
    }
    const hours = ns / 3_600_000_000_000;
    if (hours <= MAX_TIMEOUT_VALUE) {
        return std.fmt.allocPrint(alloc, "{d}H", .{hours});
    }
    return error.TimeoutOverflow;
}

fn grpcHexDigit(n: u8): u8 {
    if (n < 10) {
        return '0' + n;
    }
    return 'A' + (n - 10);
}

fn grpcParseHexVal(ch: u8): u8? {
    if (ch >= '0' and ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' and ch <= 'F') {
        return 10 + (ch - 'A');
    }
    if (ch >= 'a' and ch <= 'f') {
        return 10 + (ch - 'a');
    }
    return null;
}

pub fn percentEncodeGrpcMessage(alloc: std.mem.Allocator, msg: []const u8): ![]u8 {
    if (!simdGrpcNeedsPercentEncode(msg)) {
        return alloc.dupe(u8, msg);
    }

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    while (i < msg.len) {
        const b = msg[i];
        if (b >= 0x20 and b <= 0x7E and b != '%') {
            try out.append(alloc, b);
        } else {
            try out.append(alloc, '%');
            try out.append(alloc, grpcHexDigit(b >> 4));
            try out.append(alloc, grpcHexDigit(b & 0x0F));
        }
        i += 1;
    }
    return out.toOwnedSlice(alloc);
}

pub fn percentDecodeGrpcMessage(alloc: std.mem.Allocator, encoded: []const u8): ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    while (i < encoded.len) {
        const b = encoded[i];
        if (b == '%') {
            if (i + 2 >= encoded.len) {
                return error.InvalidPercentEncoding;
            }
            const hi = grpcParseHexVal(encoded[i + 1]) orelse return error.InvalidPercentEncoding;
            const lo = grpcParseHexVal(encoded[i + 2]) orelse return error.InvalidPercentEncoding;
            try out.append(alloc, (hi << 4) | lo);
            i += 3;
        } else {
            try out.append(alloc, b);
            i += 1;
        }
    }
    return out.toOwnedSlice(alloc);
}

pub fn encodeBinHeader(alloc: std.mem.Allocator, raw: []const u8): ![]u8 {
    const encoder = std.base64.standard.Encoder;
    const out_len = encoder.calcSize(raw.len);
    const buf = try alloc.alloc(u8, out_len);
    * = encoder.encode(buf, raw);
    return buf;
}

pub fn decodeBinHeader(alloc: std.mem.Allocator, b64: []const u8): ![]u8 {
    let has_pad = false;
    let i: usize = 0;
    while (i < b64.len) {
        if (b64[i] == '=') {
            has_pad = true;
            break;
        }
        i += 1;
    }

    if (has_pad) {
        const decoder = std.base64.standard.Decoder;
        const max_len = decoder.calcSizeForSlice(b64) catch return error.InvalidBinHeader;
        const buf = try alloc.alloc(u8, max_len);
        errdefer alloc.free(buf);
        decoder.decode(buf, b64) catch return error.InvalidBinHeader;
        return buf;
    } else {
        const decoder = std.base64.standard_no_pad.Decoder;
        const max_len = decoder.calcSizeForSlice(b64) catch return error.InvalidBinHeader;
        const buf = try alloc.alloc(u8, max_len);
        errdefer alloc.free(buf);
        decoder.decode(buf, b64) catch return error.InvalidBinHeader;
        return buf;
    }
}
