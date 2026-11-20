// Source: /data/dev/bun/src/valkey/valkey_protocol.cp
// lib/redis/resp.cp
// RESP2 / RESP3 (Redis / Valkey Serialization Protocol) Frame Parser, Stream Reader & Encoder
// Absorbed from Bun src/valkey/valkey_protocol.cp

const std = @import("std");

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
    /// Finds the index of `\r\n` starting from `start`.
    pub static fn findCrLf(buf: []const u8, start: usize): usize? {
        if (buf.len < 2 || start >= buf.len - 1) {
            return null;
        }
        let i: usize = start;
        while (i + 1 < buf.len) {
            if (buf[i] == '\r' && buf[i + 1] == '\n') {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// Parses a signed decimal i64 from an ASCII slice.
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
        let acc: i64 = 0;
        while (idx < slice.len) {
            const c = slice[idx];
            if (c < '0' || c > '9') {
                return null;
            }
            acc = acc * 10 + ((c - '0') as i64);
            idx += 1;
        }
        return if (neg) -acc else acc;
    }

    /// Parses a RESP3 Double payload (`3.14`, `-0.5`, `inf`, `-inf`, `nan`).
    pub static fn parseDouble(slice: []const u8): f64? {
        if (slice.len == 0) return null;
        if (slice == "inf" || slice == "+inf") return std.math.inf(f64);
        if (slice == "-inf") {
            const inf_val = std.math.inf(f64);
            return -inf_val;
        }
        if (slice == "nan") return std.math.nan(f64);
        return std.fmt.parseFloat(f64, slice) catch null;
    }

    /// Parses a single RESP2/RESP3 frame header or scalar value from `buf`.
    pub static fn parseFrame(buf: []const u8): RESPFrame? {
        if (buf.len < 3) {
            return null;
        }
        const kind_opt = RESPType.fromByte(buf[0]);
        if (kind_opt == null) {
            return null;
        }
        const kind = kind_opt.?;
        const crlf_opt = RESPParser.findCrLf(buf, 1);
        if (crlf_opt == null) {
            return null;
        }
        const crlf = crlf_opt.?;
        const line = buf[1..crlf];
        const after_line = crlf + 2;

        if (kind == RESPType.SimpleString || kind == RESPType.Error || kind == RESPType.BigNumber || kind == RESPType.Double) {
            return RESPFrame {
                kind: kind,
                str_val: line,
                bytes_consumed: after_line,
            };
        }

        if (kind == RESPType.Integer) {
            if (v := RESPParser.parseSignedInt(line)) {
                return RESPFrame {
                    kind: RESPType.Integer,
                    int_val: v,
                    bytes_consumed: after_line,
                };
            }
            return null;
        }

        if (kind == RESPType.Null) {
            return RESPFrame {
                kind: RESPType.Null,
                is_null: true,
                bytes_consumed: after_line,
            };
        }

        if (kind == RESPType.Boolean) {
            if (line.len != 1 || (line[0] != 't' && line[0] != 'f')) {
                return null;
            }
            return RESPFrame {
                kind: RESPType.Boolean,
                bool_val: line[0] == 't',
                bytes_consumed: after_line,
            };
        }

        if (kind == RESPType.BulkString || kind == RESPType.BlobError || kind == RESPType.VerbatimString) {
            const len_opt = RESPParser.parseSignedInt(line);
            if (len_opt == null) {
                return null;
            }
            const bulk_len = len_opt.?;
            if (bulk_len < 0) {
                return RESPFrame {
                    kind: kind,
                    is_null: true,
                    bytes_consumed: after_line,
                };
            }
            const ulen = bulk_len as usize;
            if (after_line + ulen + 2 > buf.len) {
                return null;
            }
            if (buf[after_line + ulen] != '\r' || buf[after_line + ulen + 1] != '\n') {
                return null;
            }
            return RESPFrame {
                kind: kind,
                str_val: buf[after_line .. after_line + ulen],
                bytes_consumed: after_line + ulen + 2,
            };
        }

        if (kind == RESPType.Array || kind == RESPType.Set || kind == RESPType.Push || kind == RESPType.Map || kind == RESPType.Attribute) {
            if (cnt := RESPParser.parseSignedInt(line)) {
                return RESPFrame {
                    kind: kind,
                    is_null: cnt < 0,
                    array_count: cnt,
                    bytes_consumed: after_line,
                };
            }
            return null;
        }

        return null;
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
        return _.skipFrameDepth(0);
    }

    fn skipFrameDepth(*_, depth: usize): bool {
        if (depth >= 128) return false;
        const frame = _.nextFrame() ?? return false;
        if (frame.is_null) return true;
        if (frame.kind == RESPType.Array || frame.kind == RESPType.Set || frame.kind == RESPType.Push) {
            if (frame.array_count <= 0) return true;
            let i: i64 = 0;
            while (i < frame.array_count) {
                if (!_.skipFrameDepth(depth + 1)) return false;
                i += 1;
            }
            return true;
        }
        if (frame.kind == RESPType.Map || frame.kind == RESPType.Attribute) {
            if (frame.array_count <= 0) return true;
            let i: i64 = 0;
            const pairs = frame.array_count * 2;
            while (i < pairs) {
                if (!_.skipFrameDepth(depth + 1)) return false;
                i += 1;
            }
            return true;
        }
        return true;
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
        const needed = 1 + s.len + 2;
        if (out.len < needed) { return 0; }
        out[0] = '+';
        let i: usize = 0;
        while (i < s.len) {
            out[1 + i] = s[i];
            i += 1;
        }
        out[1 + s.len] = '\r';
        out[2 + s.len] = '\n';
        return needed;
    }

    pub static fn encodeError(out: []u8, err_msg: []const u8): usize {
        const needed = 1 + err_msg.len + 2;
        if (out.len < needed) { return 0; }
        out[0] = '-';
        let i: usize = 0;
        while (i < err_msg.len) {
            out[1 + i] = err_msg[i];
            i += 1;
        }
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
            abs_val = (-val) as usize;
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
        let i: usize = 0;
        while (i < s.len) {
            out[header_len + i] = s[i];
            i += 1;
        }
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
