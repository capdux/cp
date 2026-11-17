// Source: Bun src/valkey/valkey_protocol.cp
// lib/redis/resp.cp
// RESP2 / RESP3 (Redis / Valkey Serialization Protocol) Frame Parser & Encoder
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

pub struct RESPFrame {
    kind: RESPType = RESPType.Null;
    str_val: []const u8 = "";
    int_val: i64 = 0;
    bool_val: bool = false;
    is_null: bool = false;
    array_count: i64 = 0;
    bytes_consumed: usize = 0;
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

        if (kind == RESPType.Array || kind == RESPType.Set || kind == RESPType.Push || kind == RESPType.Map) {
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
