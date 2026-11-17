// Source: Custom implementation by Antigravity AI
// lib/col/leb128.cp
// LEB128 Variable-Length Integer Encoding/Decoding

pub struct Leb128 {
    pub fn encodeUleb128(value: u64, buf: []u8) !usize {
        let n: usize = 0;
        let v = value;
        while (true) {
            let byte: u8 = @intCast(v & 0x7f);
            v >>= 7;
            if (v != 0) {
                byte |= 0x80;
                buf[n] = byte;
                n += 1;
            } else {
                buf[n] = byte;
                n += 1;
                return n;
            }
        }
    }

    pub struct Uleb128Result { value: u64, len: usize }
    pub fn decodeUleb128(buf: []const u8) !Uleb128Result {
        let value: u64 = 0;
        let shift: u6 = 0;
        let n: usize = 0;
        while (true) {
            let byte = buf[n];
            n += 1;
            value |= @as(u64, @intCast(byte & 0x7f)) << shift;
            if ((byte & 0x80) == 0) {
                return .{ .value = value, .len = n };
            }
            shift += 7;
            if (shift >= 64) return error.Overflow;
        }
    }

    pub fn encodeSleb128(value: i64, buf: []u8) !usize {
        let n: usize = 0;
        let v = value;
        while (true) {
            let byte: u8 = @intCast(v & 0x7f);
            v >>= 7;
            if ((v == 0 and (byte & 0x40) == 0) or (v == -1 and (byte & 0x40) != 0)) {
                buf[n] = byte;
                n += 1;
                return n;
            } else {
                byte |= 0x80;
                buf[n] = byte;
                n += 1;
            }
        }
    }

    pub struct Sleb128Result { value: i64, len: usize }
    pub fn decodeSleb128(buf: []const u8) !Sleb128Result {
        let value: i64 = 0;
        let shift: u6 = 0;
        let n: usize = 0;
        while (true) {
            let byte = buf[n];
            n += 1;
            value |= @as(i64, @intCast(byte & 0x7f)) << shift;
            shift += 7;
            if ((byte & 0x80) == 0) {
                if (shift < 64 and (byte & 0x40) != 0) {
                    value |= -(@as(i64, 1) << shift);
                }
                return .{ .value = value, .len = n };
            }
        }
    }
}
