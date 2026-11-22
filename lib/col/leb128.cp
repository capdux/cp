// Source: Absorbed from /data/dev/bun/src/collections/ | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/col/leb128.cp — ULEB128 (无符号) 与 SLEB128 (有符号) 变长整数编解码引擎
// 用于 DWARF 调试信息、WebAssembly 二进制指令流、二进制序列化紧凑编码
// ============================================================================

pub struct Uleb128Result {
    pub value: u64 = 0;
    pub len: usize = 0;
}

pub struct Sleb128Result {
    pub value: i64 = 0;
    pub len: usize = 0;
}

pub struct Leb128 {
    /// 计算无符号 64 位整数编码为 ULEB128 所需的字节数 (1..10)
    pub static fn uleb128Size(value: u64): usize {
        let n: usize = 1;
        let v = value >> 7;
        while (v != 0) {
            n += 1;
            v = v >> 7;
        }
        return n;
    }

    /// 计算有符号 64 位整数编码为 SLEB128 所需的字节数 (1..10)
    pub static fn sleb128Size(value: i64): usize {
        let n: usize = 0;
        let v = value;
        while (true) {
            const byte: u8 = (v & 0x7F) as u8;
            v = v >> 7;
            n += 1;
            if ((v == 0 && (byte & 0x40) == 0) || (v == -1 && (byte & 0x40) != 0)) {
                return n;
            }
        }
    }

    /// 将无符号 64 位整数编码为 ULEB128 写入 `buf`，返回写入字节数（若缓冲区不足返回 0）
    pub static fn encodeUleb128(value: u64, buf: []u8): usize {
        let n: usize = 0;
        let v = value;
        while (true) {
            if (n >= buf.len) {
                return 0;
            }
            let byte: u8 = (v & 0x7F) as u8;
            v = v >> 7;
            if (v != 0) {
                buf[n] = byte | 0x80;
                n += 1;
            } else {
                buf[n] = byte;
                n += 1;
                return n;
            }
        }
    }

    /// 从字节切片解码无符号 ULEB128，成功返回 `Uleb128Result{ value, len }`，截断或溢出返回 `null`
    pub static fn decodeUleb128(buf: []const u8): Uleb128Result? {
        let value: u64 = 0;
        let factor: u64 = 1;
        let shift: u32 = 0;
        let n: usize = 0;
        while (true) {
            if (n >= buf.len) {
                return null;
            }
            const byte = buf[n];
            n += 1;
            const low7 = (byte & 0x7F) as u64;
            value = value | (low7 * factor);
            if ((byte & 0x80) == 0) {
                return Uleb128Result {
                    value: value,
                    len: n,
                };
            }
            shift += 7;
            if (shift >= 64) {
                return null;
            }
            factor = factor << 7;
        }
    }

    /// 将有符号 64 位整数编码为 SLEB128 写入 `buf`，返回写入字节数（若缓冲区不足返回 0）
    pub static fn encodeSleb128(value: i64, buf: []u8): usize {
        let n: usize = 0;
        let v = value;
        while (true) {
            if (n >= buf.len) {
                return 0;
            }
            let byte: u8 = (v & 0x7F) as u8;
            v = v >> 7;
            if ((v == 0 && (byte & 0x40) == 0) || (v == -1 && (byte & 0x40) != 0)) {
                buf[n] = byte;
                n += 1;
                return n;
            } else {
                buf[n] = byte | 0x80;
                n += 1;
            }
        }
    }

    /// 从字节切片解码有符号 SLEB128，成功返回 `Sleb128Result{ value, len }`，截断或溢出返回 `null`
    pub static fn decodeSleb128(buf: []const u8): Sleb128Result? {
        let value: i64 = 0;
        let factor: i64 = 1;
        let shift: u32 = 0;
        let n: usize = 0;
        while (true) {
            if (n >= buf.len) {
                return null;
            }
            const byte = buf[n];
            n += 1;
            const low7: i64 = (byte & 0x7F) as i64;
            value = value + (low7 * factor);
            shift += 7;
            if ((byte & 0x80) == 0) {
                if (shift < 64 && (byte & 0x40) != 0) {
                    value = value - (factor * 128);
                }
                return Sleb128Result {
                    value: value,
                    len: n,
                };
            }
            if (shift >= 64) {
                return null;
            }
            factor = factor * 128;
        }
    }
}
