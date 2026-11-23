// Source: Absorbed from /data/dev/bun/src/valkey/valkey_protocol.cp | Synced: 2026-11-23T01:34:59Z
// Reference-hardened against redis-rs/redis-rs (BSD-3-Clause)
// lib/redis/resp.cp
// RESP2 / RESP3 (Redis / Valkey Serialization Protocol) Frame Parser, Stream Reader & Encoder
// with strict RFC/RESP3 validation, resource exhaustion bounds, and 16-lane SIMD / cap:simd acceleration.

const std = @import("std");
import { Simd } from "cap:simd";

type RespVec16 = @Vector(16, u8);

/// Maximum nested container depth (Array / Set / Map / Push / Attribute) to prevent stack exhaustion.
pub const RESP_MAX_DEPTH: usize = 64;

/// Maximum BulkString / BlobError / VerbatimString byte length (512 MiB Redis protocol limit).
pub const RESP_MAX_BULK_LEN: usize = 512 * 1024 * 1024;

/// Maximum element count in a single Array / Set / Map / Push / Attribute container.
pub const RESP_MAX_COLLECTION_LEN: usize = 1024 * 1024;

/// Strict error set returned by RESP2/RESP3 streaming parser with explicit `Incomplete` distinction.
pub err RESPError {
    Incomplete,
    InvalidType,
    InvalidInteger,
    InvalidDouble,
    InvalidBoolean,
    InvalidNull,
    InvalidBigNumber,
    InvalidVerbatim,
    InvalidLineEnding,
    InvalidLength,
    BulkTooLarge,
    CollectionTooLarge,
    MaxDepthExceeded,
}

/// SIMD-accelerated CRLF (`\r\n`) finder using `cap:simd` `Simd.indexOfByte('\r')`.
pub fn simdFindCrLf(buf: []const u8, start: usize): usize? {
    if (buf.len < 2 || start >= buf.len - 1) return null;
    let i: usize = start;
    while (i + 1 < buf.len) {
        const hit_opt = Simd.indexOfByte(buf[i..], '\r');
        if (hit_opt == null) return null;
        const cr_pos = i + hit_opt.?;
        if (cr_pos + 1 < buf.len && buf[cr_pos + 1] == '\n') {
            return cr_pos;
        }
        i = cr_pos + 1;
    }
    return null;
}

/// 16-lane `@Vector(16, u8)` SIMD scan detecting bare `\r` or `\n` inside simple lines.
pub fn simdHasCrOrLf(s: []const u8): bool {
    let i: usize = 0;
    const n = s.len;
    const v_cr: RespVec16 = @splat('\r');
    const v_lf: RespVec16 = @splat('\n');

    while (i + 16 <= n) {
        const chunk: RespVec16 = s[i..][0..16].*;
        const is_cr: u16 = @bitCast(chunk == v_cr);
        const is_lf: u16 = @bitCast(chunk == v_lf);
        if ((is_cr | is_lf) != 0) return true;
        i += 16;
    }

    while (i < n) : (i += 1) {
        if (s[i] == '\r' || s[i] == '\n') return true;
    }
    return false;
}

/// 16-lane `@Vector(16, u8)` SIMD payload copy for BulkString and RESP frame encoding.
pub fn simdCopyPayload(dst: []u8, src: []const u8): usize {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const chunk: RespVec16 = src[i..][0..16].*;
        dst[i..][0..16].* = chunk;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

fn isValidBigNumber(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    if (s[0] == '-' || s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) return false;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

pub enum RESPType: u8 {
    // RESP2 types
    SimpleString = '+',
    Error = '-',
    Integer = ':',
    BulkString = '$',
    Array = '*',

    // RESP3 types
    Null = '_',
    Double = ',',
    Boolean = '#',
    BlobError = '!',
    VerbatimString = '=',
    Map = '%',
    Set = '~',
    Attribute = '|',
    Push = '>',
    BigNumber = '(',

    pub static fn fromByte(byte: u8): RESPType? {
        if (byte == '+') { return RESPType.SimpleString; }
        if (byte == '-') { return RESPType.Error; }
        if (byte == ':') { return RESPType.Integer; }
        if (byte == '$') { return RESPType.BulkString; }
        if (byte == '*') { return RESPType.Array; }
        if (byte == '_') { return RESPType.Null; }
        if (byte == ',') { return RESPType.Double; }
        if (byte == '#') { return RESPType.Boolean; }
        if (byte == '!') { return RESPType.BlobError; }
        if (byte == '=') { return RESPType.VerbatimString; }
        if (byte == '%') { return RESPType.Map; }
        if (byte == '~') { return RESPType.Set; }
        if (byte == '|') { return RESPType.Attribute; }
        if (byte == '>') { return RESPType.Push; }
        if (byte == '(') { return RESPType.BigNumber; }
        return null;
    }
}

import { VerbatimString } from "./protocol.cp";

pub struct RESPFrame {
    pub kind: RESPType = RESPType.Null;
    pub str_val: []const u8 = "";
    pub int_val: i64 = 0;
    pub bool_val: bool = false;
    pub is_null: bool = false;
    pub array_count: i64 = 0;
    pub bytes_consumed: usize = 0;

    pub fn asDouble(): f64? {
        if (_.kind != RESPType.Double) return null;
        return RESPParser.parseDouble(_.str_val);
    }

    pub fn asVerbatimString(): VerbatimString? {
        if (_.kind != RESPType.VerbatimString) return null;
        return VerbatimString.parse(_.str_val);
    }
}

pub struct RESPParser {
    /// Finds the index of `\r\n` starting from `start` using SIMD acceleration.
    pub static fn findCrLf(buf: []const u8, start: usize): usize? {
        return simdFindCrLf(buf, start);
    }

    /// Parses a signed decimal i64 from an ASCII slice with strict overflow guards (including `i64::MIN`).
    pub static fn parseSignedInt(slice: []const u8): i64? {
        if (slice.len == 0) {
            return null;
        }
        let neg: bool = false;
        let idx: usize = 0;
        if (slice[0] == '-') {
            neg = true;
            idx = 1;
        } else if (slice[0] == '+') {
            idx = 1;
        }
        if (idx >= slice.len) {
            return null;
        }
        const limit: u64 = if (neg) (9223372036854775808 as u64) else (9223372036854775807 as u64);
        let acc: u64 = 0;
        while (idx < slice.len) {
            const c = slice[idx];
            if (c < '0' || c > '9') {
                return null;
            }
            const digit: u64 = (c - '0') as u64;
            if (acc > (limit - digit) / 10) {
                return null;
            }
            acc = acc * 10 + digit;
            idx += 1;
        }
        if (neg) {
            if (acc == (9223372036854775808 as u64)) {
                return std.math.minInt(i64);
            }
            return -(acc as i64);
        }
        return acc as i64;
    }

    /// Parses a RESP3 Double payload (`3.14`, `-0.5`, `inf`, `+inf`, `-inf`, `nan`).
    pub static fn parseDouble(slice: []const u8): f64? {
        if (slice.len == 0 || simdHasCrOrLf(slice)) return null;
        if (std.mem.eql(u8, slice, "inf") || std.mem.eql(u8, slice, "+inf")) return std.math.inf(f64);
        if (std.mem.eql(u8, slice, "-inf")) {
            const inf_val = std.math.inf(f64);
            return -inf_val;
        }
        if (std.mem.eql(u8, slice, "nan")) return std.math.nan(f64);
        return std.fmt.parseFloat(f64, slice) catch null;
    }

    /// Strict RESP2/RESP3 frame parser with explicit `RESPError.Incomplete` vs protocol error distinction,
    /// bare CR/LF rejection, length bounds, and full RESP3 scalar/header validation.
    pub static fn parseFrameStrict(buf: []const u8): RESPFrame ! RESPError {
        if (buf.len == 0) return err.Incomplete;
        const kind = RESPType.fromByte(buf[0]) ?? return err.InvalidType;
        if (buf.len < 3) return err.Incomplete;

        const crlf_opt = simdFindCrLf(buf, 1);
        if (crlf_opt == null) {
            // Check if a bare '\r' followed by non-'\n' already exists before end of buffer
            let k: usize = 1;
            while (k + 1 < buf.len) : (k += 1) {
                if (buf[k] == '\r' && buf[k + 1] != '\n') return err.InvalidLineEnding;
                if (buf[k] == '\n') return err.InvalidLineEnding;
            }
            return err.Incomplete;
        }
        const crlf = crlf_opt.?;
        const line = buf[1..crlf];
        const after_line = crlf + 2;

        // Reject bare '\r' or '\n' embedded inside the header line
        if (simdHasCrOrLf(line)) {
            return err.InvalidLineEnding;
        }

        if (kind == RESPType.SimpleString || kind == RESPType.Error) {
            return RESPFrame{
                kind: kind,
                str_val: line,
                bytes_consumed: after_line,
            };
        }

        if (kind == RESPType.BigNumber) {
            if (!isValidBigNumber(line)) return err.InvalidBigNumber;
            return RESPFrame{
                kind: kind,
                str_val: line,
                bytes_consumed: after_line,
            };
        }

        if (kind == RESPType.Double) {
            if (RESPParser.parseDouble(line) == null) return err.InvalidDouble;
            return RESPFrame{
                kind: kind,
                str_val: line,
                bytes_consumed: after_line,
            };
        }

        if (kind == RESPType.Integer) {
            const v = RESPParser.parseSignedInt(line) ?? return err.InvalidInteger;
            return RESPFrame{
                kind: RESPType.Integer,
                int_val: v,
                bytes_consumed: after_line,
            };
        }

        if (kind == RESPType.Null) {
            if (line.len != 0) return err.InvalidNull;
            return RESPFrame{
                kind: RESPType.Null,
                is_null: true,
                bytes_consumed: after_line,
            };
        }

        if (kind == RESPType.Boolean) {
            if (line.len != 1 || (line[0] != 't' && line[0] != 'f')) {
                return err.InvalidBoolean;
            }
            return RESPFrame{
                kind: RESPType.Boolean,
                bool_val: line[0] == 't',
                bytes_consumed: after_line,
            };
        }

        if (kind == RESPType.BulkString || kind == RESPType.BlobError || kind == RESPType.VerbatimString) {
            const bulk_len = RESPParser.parseSignedInt(line) ?? return err.InvalidLength;
            if (bulk_len < 0) {
                if (bulk_len != -1) return err.InvalidLength;
                return RESPFrame{
                    kind: kind,
                    is_null: true,
                    bytes_consumed: after_line,
                };
            }
            const ulen = bulk_len as usize;
            if (ulen > RESP_MAX_BULK_LEN) return err.BulkTooLarge;
            if (ulen > buf.len - after_line || buf.len - after_line - ulen < 2) {
                return err.Incomplete;
            }
            if (buf[after_line + ulen] != '\r' || buf[after_line + ulen + 1] != '\n') {
                return err.InvalidLineEnding;
            }
            const payload = buf[after_line .. after_line + ulen];
            if (kind == RESPType.VerbatimString) {
                if (payload.len < 4 || payload[3] != ':') return err.InvalidVerbatim;
            }
            return RESPFrame{
                kind: kind,
                str_val: payload,
                bytes_consumed: after_line + ulen + 2,
            };
        }

        if (kind == RESPType.Array || kind == RESPType.Set || kind == RESPType.Push || kind == RESPType.Map || kind == RESPType.Attribute) {
            const cnt = RESPParser.parseSignedInt(line) ?? return err.InvalidLength;
            if (cnt < 0) {
                if (cnt != -1) return err.InvalidLength;
                return RESPFrame{
                    kind: kind,
                    is_null: true,
                    array_count: cnt,
                    bytes_consumed: after_line,
                };
            }
            const ucnt = cnt as usize;
            if (ucnt > RESP_MAX_COLLECTION_LEN) return err.CollectionTooLarge;
            return RESPFrame{
                kind: kind,
                is_null: false,
                array_count: cnt,
                bytes_consumed: after_line,
            };
        }

        return err.InvalidType;
    }

    /// Parses a single RESP2/RESP3 frame header or scalar value from `buf`.
    pub static fn parseFrame(buf: []const u8): RESPFrame? {
        return RESPParser.parseFrameStrict(buf) catch null;
    }
}

/// Zero-copy streaming RESP2/RESP3 frame reader state machine
pub struct ValkeyReader {
    pub buffer: []const u8 = "";
    pub pos: usize = 0;

    pub static fn init(buffer: []const u8): ValkeyReader {
        return ValkeyReader {
            buffer: buffer,
            pos: 0,
        };
    }

    pub fn reset(*_) {
        _.pos = 0;
    }

    pub fn hasMore(): bool {
        return _.pos < _.buffer.len;
    }

    pub fn remaining(): []const u8 {
        if (_.pos >= _.buffer.len) return "";
        return _.buffer[_.pos.._.buffer.len];
    }

    pub fn nextFrame(*_): RESPFrame? {
        if (_.pos >= _.buffer.len) return null;
        if (frame := RESPParser.parseFrame(_.buffer[_.pos.._.buffer.len])) {
            _.pos += frame.bytes_consumed;
            return frame;
        }
        return null;
    }

    /// Skips one complete RESP2/RESP3 frame (including nested Array/Set/Push/Map/Attribute items).
    pub fn skipFrame(*_): bool {
        const start_pos = _.pos;
        const ok = _.validateCompleteDepth(0) catch {
            _.pos = start_pos;
            return false;
        };
        return ok > 0;
    }

    /// Strictly validates and consumes one complete RESP2/RESP3 frame (recursively enforcing `RESP_MAX_DEPTH = 64`),
    /// returning total bytes consumed or `RESPError` (preserving `pos` on error/Incomplete).
    pub fn validateCompleteFrame(*_): usize ! RESPError {
        const start_pos = _.pos;
        const consumed = _.validateCompleteDepth(0) catch |e| {
            _.pos = start_pos;
            return e;
        };
        return consumed;
    }

    fn validateCompleteDepth(*_, depth: usize): usize ! RESPError {
        if (depth >= RESP_MAX_DEPTH) return err.MaxDepthExceeded;
        if (_.pos >= _.buffer.len) return err.Incomplete;
        const frame = RESPParser.parseFrameStrict(_.buffer[_.pos.._.buffer.len]) catch |e| {
            return e;
        };
        _.pos += frame.bytes_consumed;
        let total = frame.bytes_consumed;
        if (frame.is_null) return total;

        if (frame.kind == RESPType.Array || frame.kind == RESPType.Set || frame.kind == RESPType.Push) {
            if (frame.array_count <= 0) return total;
            const ucnt = frame.array_count as usize;
            let i: usize = 0;
            while (i < ucnt) : (i += 1) {
                const sub = _.validateCompleteDepth(depth + 1) catch |e| {
                    return e;
                };
                total += sub;
            }
            return total;
        }
        if (frame.kind == RESPType.Map || frame.kind == RESPType.Attribute) {
            if (frame.array_count <= 0) return total;
            const ucnt = frame.array_count as usize;
            if (ucnt > RESP_MAX_COLLECTION_LEN / 2) return err.CollectionTooLarge;
            const pairs = ucnt * 2;
            let i: usize = 0;
            while (i < pairs) : (i += 1) {
                const sub = _.validateCompleteDepth(depth + 1) catch |e| {
                    return e;
                };
                total += sub;
            }
            return total;
        }
        return total;
    }
}

pub struct RESPEncoder {
    /// Writes an unsigned decimal integer into `out` and returns the number of bytes written.
    pub static fn writeUint(out: []u8, val: usize): usize {
        if (val == 0) {
            if (out.len < 1) { return 0; }
            out[0] = '0';
            return 1;
        }
        let tmp: [24]u8 = [0; 24];
        let len: usize = 0;
        let v = val;
        while (v > 0) {
            tmp[len] = (48 as u8) + ((v % 10) as u8);
            len += 1;
            v /= 10;
        }
        if (out.len < len) {
            return 0;
        }
        let i: usize = 0;
        while (i < len) {
            out[i] = tmp[len - 1 - i];
            i += 1;
        }
        return len;
    }

    pub static fn encodeSimpleString(out: []u8, s: []const u8): usize {
        if (simdHasCrOrLf(s)) return 0;
        const needed = 1 + s.len + 2;
        if (out.len < needed) { return 0; }
        out[0] = '+';
        const c = simdCopyPayload(out[1 .. 1 + s.len], s);
        if (c != s.len) return 0;
        out[1 + s.len] = '\r';
        out[2 + s.len] = '\n';
        return needed;
    }

    pub static fn encodeError(out: []u8, err_msg: []const u8): usize {
        if (simdHasCrOrLf(err_msg)) return 0;
        const needed = 1 + err_msg.len + 2;
        if (out.len < needed) { return 0; }
        out[0] = '-';
        const c = simdCopyPayload(out[1 .. 1 + err_msg.len], err_msg);
        if (c != err_msg.len) return 0;
        out[1 + err_msg.len] = '\r';
        out[2 + err_msg.len] = '\n';
        return needed;
    }

    pub static fn encodeInteger(out: []u8, val: i64): usize {
        if (out.len < 4) { return 0; }
        out[0] = ':';
        let pos: usize = 1;
        let abs_val: usize = 0;
        if (val < 0) {
            out[pos] = '-';
            pos += 1;
            if (val == std.math.minInt(i64)) {
                abs_val = 9223372036854775808 as usize;
            } else {
                abs_val = (-val) as usize;
            }
        } else {
            abs_val = val as usize;
        }
        const dlen = RESPEncoder.writeUint(out[pos..], abs_val);
        if (dlen == 0 || pos + dlen + 2 > out.len) { return 0; }
        pos += dlen;
        out[pos] = '\r';
        out[pos + 1] = '\n';
        return pos + 2;
    }

    pub static fn encodeBoolean(out: []u8, val: bool): usize {
        if (out.len < 4) { return 0; }
        out[0] = '#';
        out[1] = if (val) 't' else 'f';
        out[2] = '\r';
        out[3] = '\n';
        return 4;
    }

    pub static fn encodeNull(out: []u8): usize {
        if (out.len < 3) { return 0; }
        out[0] = '_';
        out[1] = '\r';
        out[2] = '\n';
        return 3;
    }

    pub static fn encodeNullBulkString(out: []u8): usize {
        if (out.len < 5) { return 0; }
        out[0] = '$';
        out[1] = '-';
        out[2] = '1';
        out[3] = '\r';
        out[4] = '\n';
        return 5;
    }

    pub static fn encodeBulkString(out: []u8, s: []const u8): usize {
        if (out.len < 4) { return 0; }
        out[0] = '$';
        const dlen = RESPEncoder.writeUint(out[1..], s.len);
        if (dlen == 0) { return 0; }
        const header_len = 1 + dlen + 2;
        const total = header_len + s.len + 2;
        if (out.len < total) { return 0; }
        out[1 + dlen] = '\r';
        out[2 + dlen] = '\n';
        const c = simdCopyPayload(out[header_len .. header_len + s.len], s);
        if (c != s.len) return 0;
        out[header_len + s.len] = '\r';
        out[header_len + s.len + 1] = '\n';
        return total;
    }

    pub static fn encodeArrayHeader(out: []u8, count: usize): usize {
        if (out.len < 4) { return 0; }
        out[0] = '*';
        const dlen = RESPEncoder.writeUint(out[1..], count);
        if (dlen == 0 || out.len < 1 + dlen + 2) { return 0; }
        out[1 + dlen] = '\r';
        out[2 + dlen] = '\n';
        return 1 + dlen + 2;
    }

    pub static fn encodeMapHeader(out: []u8, count: usize): usize {
        if (out.len < 4) { return 0; }
        out[0] = '%';
        const dlen = RESPEncoder.writeUint(out[1..], count);
        if (dlen == 0 || out.len < 1 + dlen + 2) { return 0; }
        out[1 + dlen] = '\r';
        out[2 + dlen] = '\n';
        return 1 + dlen + 2;
    }

    pub static fn encodeSetHeader(out: []u8, count: usize): usize {
        if (out.len < 4) { return 0; }
        out[0] = '~';
        const dlen = RESPEncoder.writeUint(out[1..], count);
        if (dlen == 0 || out.len < 1 + dlen + 2) { return 0; }
        out[1 + dlen] = '\r';
        out[2 + dlen] = '\n';
        return 1 + dlen + 2;
    }

    pub static fn encodePushHeader(out: []u8, count: usize): usize {
        if (out.len < 4) { return 0; }
        out[0] = '>';
        const dlen = RESPEncoder.writeUint(out[1..], count);
        if (dlen == 0 || out.len < 1 + dlen + 2) { return 0; }
        out[1 + dlen] = '\r';
        out[2 + dlen] = '\n';
        return 1 + dlen + 2;
    }

    pub static fn encodeAttributeHeader(out: []u8, count: usize): usize {
        if (out.len < 4) { return 0; }
        out[0] = '|';
        const dlen = RESPEncoder.writeUint(out[1..], count);
        if (dlen == 0 || out.len < 1 + dlen + 2) { return 0; }
        out[1 + dlen] = '\r';
        out[2 + dlen] = '\n';
        return 1 + dlen + 2;
    }

    pub static fn encodeDoubleStr(out: []u8, val_str: []const u8): usize {
        const needed = 1 + val_str.len + 2;
        if (out.len < needed) { return 0; }
        out[0] = ',';
        let i: usize = 0;
        while (i < val_str.len) {
            out[1 + i] = val_str[i];
            i += 1;
        }
        out[1 + val_str.len] = '\r';
        out[2 + val_str.len] = '\n';
        return needed;
    }

    pub static fn encodeBigNumber(out: []u8, num_str: []const u8): usize {
        const needed = 1 + num_str.len + 2;
        if (out.len < needed) { return 0; }
        out[0] = '(';
        let i: usize = 0;
        while (i < num_str.len) {
            out[1 + i] = num_str[i];
            i += 1;
        }
        out[1 + num_str.len] = '\r';
        out[2 + num_str.len] = '\n';
        return needed;
    }

    pub static fn encodeBlobError(out: []u8, err_msg: []const u8): usize {
        if (out.len < 4) { return 0; }
        out[0] = '!';
        const dlen = RESPEncoder.writeUint(out[1..], err_msg.len);
        if (dlen == 0) { return 0; }
        const header_len = 1 + dlen + 2;
        const total = header_len + err_msg.len + 2;
        if (out.len < total) { return 0; }
        out[1 + dlen] = '\r';
        out[2 + dlen] = '\n';
        let i: usize = 0;
        while (i < err_msg.len) {
            out[header_len + i] = err_msg[i];
            i += 1;
        }
        out[header_len + err_msg.len] = '\r';
        out[header_len + err_msg.len + 1] = '\n';
        return total;
    }

    pub static fn encodeVerbatimString(out: []u8, fmt3: []const u8, content: []const u8): usize {
        if (fmt3.len != 3 || out.len < 8) { return 0; }
        const payload_len = 4 + content.len;
        out[0] = '=';
        const dlen = RESPEncoder.writeUint(out[1..], payload_len);
        if (dlen == 0) { return 0; }
        const header_len = 1 + dlen + 2;
        const total = header_len + payload_len + 2;
        if (out.len < total) { return 0; }
        out[1 + dlen] = '\r';
        out[2 + dlen] = '\n';
        out[header_len] = fmt3[0];
        out[header_len + 1] = fmt3[1];
        out[header_len + 2] = fmt3[2];
        out[header_len + 3] = ':';
        let i: usize = 0;
        while (i < content.len) {
            out[header_len + 4 + i] = content[i];
            i += 1;
        }
        out[header_len + payload_len] = '\r';
        out[header_len + payload_len + 1] = '\n';
        return total;
    }

    /// Encodes an arbitrary N-argument Redis command (`*N\r\n$...\r\n...`) into `out`.
    pub static fn encodeCommand(out: []u8, args: []const []const u8): usize {
        if (args.len == 0) { return 0; }
        let pos: usize = RESPEncoder.encodeArrayHeader(out, args.len);
        if (pos == 0) { return 0; }
        let i: usize = 0;
        while (i < args.len) {
            const n = RESPEncoder.encodeBulkString(out[pos..], args[i]);
            if (n == 0) { return 0; }
            pos += n;
            i += 1;
        }
        return pos;
    }

    /// Encodes a 2-token Redis command (e.g. `GET key`) into `out`.
    pub static fn encodeCommand2(out: []u8, cmd: []const u8, arg1: []const u8): usize {
        let pos: usize = RESPEncoder.encodeArrayHeader(out, 2);
        if (pos == 0) { return 0; }
        const n1 = RESPEncoder.encodeBulkString(out[pos..], cmd);
        if (n1 == 0) { return 0; }
        pos += n1;
        const n2 = RESPEncoder.encodeBulkString(out[pos..], arg1);
        if (n2 == 0) { return 0; }
        return pos + n2;
    }

    /// Encodes a 3-token Redis command (e.g. `SET key val`) into `out`.
    pub static fn encodeCommand3(out: []u8, cmd: []const u8, arg1: []const u8, arg2: []const u8): usize {
        let pos: usize = RESPEncoder.encodeArrayHeader(out, 3);
        if (pos == 0) { return 0; }
        const n1 = RESPEncoder.encodeBulkString(out[pos..], cmd);
        if (n1 == 0) { return 0; }
        pos += n1;
        const n2 = RESPEncoder.encodeBulkString(out[pos..], arg1);
        if (n2 == 0) { return 0; }
        pos += n2;
        const n3 = RESPEncoder.encodeBulkString(out[pos..], arg2);
        if (n3 == 0) { return 0; }
        return pos + n3;
    }
}
