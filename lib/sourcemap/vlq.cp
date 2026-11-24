// Source: Absorbed from /data/dev/bun/src/sourcemap/VLQ.cp | Synced: 2026-11-24T04:33:36Z
// ============================================================================
// lib/sourcemap/vlq.cp — Base64 VLQ (Variable-Length Quantity) 编解码器
// 吸收自 Bun src/sourcemap/VLQ.cp
// ============================================================================

const std = @import("std");

pub const BASE64_CHARS: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/// Base64 VLQ 编码结果（最多 7 字节栈内联，覆盖完整 i32 范围 [-2147483648, 2147483647]）
pub struct VLQ {
    bytes: [7]u8 = [0, 0, 0, 0, 0, 0, 0],
    len: u8 = 0,

    pub static fn zero(): VLQ {
        return VLQ{
            bytes: ['A', 0, 0, 0, 0, 0, 0],
            len: 1,
        };
    }

    pub fn slice(*_): []const u8 {
        let n = _.len as usize;
        return _.bytes[0..n];
    }

    pub fn writeTo(writer: auto): !void {
        try writer.writeAll(_.bytes[0 .. _.len as usize]);
    }

    pub static fn encode(value: i32): VLQ {
        let out = VLQ{
            bytes: [0, 0, 0, 0, 0, 0, 0],
            len: 0,
        };
        let wide: i64 = value as i64;
        let mag: u64 = if (wide >= 0) (wide as u64) else ((-wide) as u64);
        let vlq: u64 = (mag << 1) | (if (wide < 0) (1 as u64) else (0 as u64));

        let step: usize = 0;
        while (step < 7) {
            let digit: u64 = vlq & 31;
            vlq = vlq >> 5;
            if (vlq != 0) {
                digit = digit | 32;
            }
            let idx = out.len as usize;
            out.bytes[idx] = BASE64_CHARS[digit as usize];
            out.len += 1;
            if (vlq == 0) {
                return out;
            }
            step += 1;
        }
        return out;
    }

    pub static fn decode(encoded: []const u8, start: usize): VLQResult {
        let shift: u6 = 0;
        let vlq: u64 = 0;
        let pos = start;
        let step: usize = 0;

        while (pos < encoded.len && step < 7) {
            let b = encoded[pos];
            let sextet_opt = decodeBase64Sextet(b);
            if (sextet_opt == null) {
                break;
            }
            let digit = (sextet_opt ?? 0) as u64;
            vlq = vlq | ((digit & 31) << shift);
            pos += 1;
            step += 1;

            if ((digit & 32) == 0) {
                let mag: u64 = vlq >> 1;
                let signed_wide: i64 = if ((vlq & 1) == 0) (mag as i64) else (-(mag as i64));
                if (signed_wide < -2147483648 || signed_wide > 2147483647) {
                    return VLQResult{
                        value: 0,
                        next_offset: pos,
                        valid: false,
                    };
                }
                return VLQResult{
                    value: signed_wide as i32,
                    next_offset: pos,
                    valid: true,
                };
            }
            shift += 5;
        }

        return VLQResult{
            value: 0,
            next_offset: pos,
            valid: false,
        };
    }

    pub static fn decodeAssumeValid(encoded: []const u8, start: usize): VLQResult {
        return VLQ.decode(encoded, start);
    }

    pub static fn encodeValues(values: []const i32, out: []u8): usize {
        let out_idx: usize = 0;
        for (val in values) {
            let encoded = VLQ.encode(val);
            const s = encoded.slice();
            if (out_idx + s.len > out.len) break;
            @memcpy(out[out_idx .. out_idx + s.len], s);
            out_idx += s.len;
        }
        return out_idx;
    }

    pub static fn decodeValues(encoded: []const u8, out: []i32): usize {
        let pos: usize = 0;
        let count: usize = 0;
        while (pos < encoded.len && count < out.len) {
            const res = VLQ.decode(encoded, pos);
            if (!res.valid) break;
            out[count] = res.value;
            count += 1;
            pos = res.next_offset;
        }
        return count;
    }

    /// 编码任意 64 位有符号整数（最多 13 字节，支持超大偏移与边界）
    pub static fn encode64(value: i64): VLQ64 {
        let out = VLQ64{
            bytes: [0; 13],
            len: 0,
        };
        let mag: u64 = if (value >= 0)
            (value as u64)
        else if (value == std.math.minInt(i64))
            (@as(u64, 1) << 63)
        else
            ((-value) as u64);
        let is_neg: bool = value < 0;

        // First 4 value bits + sign bit
        let first_digit: u64 = ((mag & 0x0F) << 1) | (if (is_neg) (1 as u64) else (0 as u64));
        let rem: u64 = mag >> 4;
        if (rem != 0) {
            first_digit = first_digit | 32;
        }
        out.bytes[0] = BASE64_CHARS[first_digit as usize];
        out.len = 1;

        let step: usize = 1;
        while (rem != 0 && step < 13) {
            let digit: u64 = rem & 31;
            rem = rem >> 5;
            if (rem != 0) {
                digit = digit | 32;
            }
            out.bytes[step] = BASE64_CHARS[digit as usize];
            out.len += 1;
            step += 1;
        }
        return out;
    }

    /// 解码 64 位有符号 Base64 VLQ 整数
    pub static fn decode64(encoded: []const u8, start: usize): VLQ64Result {
        let shift: u7 = 0;
        let mag: u64 = 0;
        let is_neg: bool = false;
        let pos = start;
        let step: usize = 0;

        while (pos < encoded.len && step < 13) {
            let b = encoded[pos];
            let sextet_opt = decodeBase64Sextet(b);
            if (sextet_opt == null) {
                break;
            }
            let digit = (sextet_opt ?? 0) as u64;
            pos += 1;
            if (step == 0) {
                is_neg = (digit & 1) != 0;
                mag = (digit & 30) >> 1;
                shift = 4;
            } else {
                if (shift >= 64) {
                    return VLQ64Result{ value: 0, next_offset: pos, valid: false };
                }
                mag = mag | ((digit & 31) << @as(u6, @intCast(shift)));
                shift += 5;
            }
            step += 1;

            if ((digit & 32) == 0) {
                let signed_val: i64 = if (!is_neg)
                    @bitCast(mag)
                else if (mag == (@as(u64, 1) << 63))
                    std.math.minInt(i64)
                else
                    -(@as(i64, @intCast(mag)));
                return VLQ64Result{
                    value: signed_val,
                    next_offset: pos,
                    valid: true,
                };
            }
        }

        return VLQ64Result{
            value: 0,
            next_offset: pos,
            valid: false,
        };
    }
}

pub struct VLQResult {
    value: i32 = 0,
    next_offset: usize = 0,
    valid: bool = false,
}

pub struct VLQ64 {
    bytes: [13]u8 = [0; 13],
    len: u8 = 0,

    pub fn slice(*_): []const u8 {
        let n = _.len as usize;
        return _.bytes[0..n];
    }
}

pub struct VLQ64Result {
    value: i64 = 0,
    next_offset: usize = 0,
    valid: bool = false,
}

pub fn decodeBase64Sextet(b: u8): u8? {
    if (b >= 65 && b <= 90) { return b - 65; }         // 'A'..'Z' -> 0..25
    if (b >= 97 && b <= 122) { return b - 97 + 26; }   // 'a'..'z' -> 26..51
    if (b >= 48 && b <= 57) { return b - 48 + 52; }    // '0'..'9' -> 52..61
    if (b == 43) { return 62; }                        // '+' -> 62
    if (b == 47) { return 63; }                        // '/' -> 63
    return null;
}
