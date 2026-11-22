// Origin: Pure-CP Stdlib (Zero-Alloc Parity SCALE Deterministic Codec) | Created: 2026-11-22

/// SCALE 特有单字节三态 `Option<bool>` 编码枚举（0x00=None, 0x01=Some(true), 0x02=Some(false)）
pub enum ScaleOptionBool: u8 {
    none = 0,
    some_true = 1,
    some_false = 2,
}

/// 计算 `Compact<u64>` 规范最小编码的字节长度（1 / 2 / 4 / 5..9）
pub fn compactU64Len(v: u64): usize {
    if (v <= 63) return 1;
    if (v <= 16383) return 2;
    if (v <= 1073741823) return 4;
    return minLeBytesU64(v) + 1;
}

fn minLeBytesU64(v: u64): usize {
    let x = v;
    let n: usize = 0;
    while (x > 0) {
        n += 1;
        x = x >> 8;
    }
    return if (n < 4) 4 else n;
}

/// 零堆分配 SCALE 二进制写入器（写入用户提供的 `[]u8` 缓冲区，空间不足原子返回 `false`）
pub struct ScaleWriter {
    buf: []u8,
    pos: usize = 0,

    static fn init(buf: []u8): ScaleWriter {
        return ScaleWriter {
            buf: buf,
            pos: 0,
        };
    }

    fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    fn remaining(): usize {
        return _.buf.len - _.pos;
    }

    fn writeU8(*_, v: u8): bool {
        if (_.buf.len - _.pos < 1) return false;
        _.buf[_.pos] = v;
        _.pos += 1;
        return true;
    }

    fn writeI8(*_, v: i8): bool {
        return _.writeU8(v as u8);
    }

    fn writeBool(*_, v: bool): bool {
        return _.writeU8(if (v) 1 else 0);
    }

    fn writeOptionBool(*_, v: bool?): bool {
        if (b := v) {
            return _.writeU8(if (b) 1 else 2);
        }
        return _.writeU8(0);
    }

    fn writeOptionTag(*_, is_some: bool): bool {
        return _.writeU8(if (is_some) 1 else 0);
    }

    fn writeResultTag(*_, is_ok: bool): bool {
        // SCALE Result: 0x00 = Ok, 0x01 = Err
        return _.writeU8(if (is_ok) 0 else 1);
    }

    fn writeEnumTag(*_, variant: u8): bool {
        return _.writeU8(variant);
    }

    fn writeU16(*_, v: u16): bool {
        if (_.buf.len - _.pos < 2) return false;
        _.buf[_.pos] = (v & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.pos += 2;
        return true;
    }

    fn writeI16(*_, v: i16): bool {
        return _.writeU16(v as u16);
    }

    fn writeU32(*_, v: u32): bool {
        if (_.buf.len - _.pos < 4) return false;
        _.buf[_.pos] = (v & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        _.pos += 4;
        return true;
    }

    fn writeI32(*_, v: i32): bool {
        return _.writeU32(v as u32);
    }

    fn writeU64(*_, v: u64): bool {
        if (_.buf.len - _.pos < 8) return false;
        let i: usize = 0;
        while (i < 8) {
            const shift: u6 = (i * 8) as u6;
            _.buf[_.pos + i] = ((v >> shift) & 0xFF) as u8;
            i += 1;
        }
        _.pos += 8;
        return true;
    }

    fn writeI64(*_, v: i64): bool {
        return _.writeU64(v as u64);
    }

    /// 写入规范最小编码的 `Compact<u64>`
    fn writeCompactU64(*_, v: u64): bool {
        const need = compactU64Len(v);
        if (need > _.buf.len - _.pos) return false;

        if (v <= 63) {
            _.buf[_.pos] = ((v << 2) & 0xFF) as u8;
            _.pos += 1;
            return true;
        }
        if (v <= 16383) {
            const enc: u16 = ((v as u16) << 2) | 0x01;
            _.buf[_.pos] = (enc & 0xFF) as u8;
            _.buf[_.pos + 1] = ((enc >> 8) & 0xFF) as u8;
            _.pos += 2;
            return true;
        }
        if (v <= 1073741823) {
            const enc: u32 = ((v as u32) << 2) | 0x02;
            _.buf[_.pos] = (enc & 0xFF) as u8;
            _.buf[_.pos + 1] = ((enc >> 8) & 0xFF) as u8;
            _.buf[_.pos + 2] = ((enc >> 16) & 0xFF) as u8;
            _.buf[_.pos + 3] = ((enc >> 24) & 0xFF) as u8;
            _.pos += 4;
            return true;
        }

        const n = minLeBytesU64(v);
        _.buf[_.pos] = (((n - 4) as u8) << 2) | 0x03;
        let i: usize = 0;
        while (i < n) {
            const shift: u6 = (i * 8) as u6;
            _.buf[_.pos + 1 + i] = ((v >> shift) & 0xFF) as u8;
            i += 1;
        }
        _.pos += n + 1;
        return true;
    }

    /// 写入字节切片（`Compact<u64>` 长度前缀 + 原始字节）
    fn writeBytes(*_, data: []const u8): bool {
        const prefix_len = compactU64Len(data.len as u64);
        if (prefix_len > _.buf.len - _.pos) return false;
        if (data.len > _.buf.len - _.pos - prefix_len) return false;

        if (!_.writeCompactU64(data.len as u64)) return false;
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.pos + i] = data[i];
            i += 1;
        }
        _.pos += data.len;
        return true;
    }

    /// 写入 UTF-8 字符串（与 `writeBytes` 等价）
    fn writeString(*_, s: []const u8): bool {
        return _.writeBytes(s);
    }
}

/// 零拷贝只读 SCALE 解码游标
pub struct ScaleReader {
    data: []const u8,
    pos: usize = 0,

    static fn init(data: []const u8): ScaleReader {
        return ScaleReader {
            data: data,
            pos: 0,
        };
    }

    fn remaining(): usize {
        return _.data.len - _.pos;
    }

    fn isEof(): bool {
        return _.pos >= _.data.len;
    }

    fn readU8(*_): u8? {
        if (_.data.len - _.pos < 1) return null;
        const v = _.data[_.pos];
        _.pos += 1;
        return v;
    }

    fn readI8(*_): i8? {
        const v = _.readU8() ?? return null;
        return v as i8;
    }

    fn readBool(*_): bool? {
        if (_.data.len - _.pos < 1) return null;
        const b = _.data[_.pos];
        if (b > 1) return null;
        _.pos += 1;
        return b == 1;
    }

    fn readOptionBool(*_): ScaleOptionBool? {
        if (_.data.len - _.pos < 1) return null;
        const b = _.data[_.pos];
        if (b > 2) return null;
        _.pos += 1;
        return b as ScaleOptionBool;
    }

    fn readOptionTag(*_): bool? {
        return _.readBool();
    }

    fn readResultTag(*_): bool? {
        if (_.data.len - _.pos < 1) return null;
        const b = _.data[_.pos];
        if (b > 1) return null;
        _.pos += 1;
        // 0x00 = Ok (true), 0x01 = Err (false)
        return b == 0;
    }

    fn readEnumTag(*_): u8? {
        return _.readU8();
    }

    fn readU16(*_): u16? {
        if (_.data.len - _.pos < 2) return null;
        const v: u16 = (_.data[_.pos] as u16) | ((_.data[_.pos + 1] as u16) << 8);
        _.pos += 2;
        return v;
    }

    fn readI16(*_): i16? {
        const v = _.readU16() ?? return null;
        return v as i16;
    }

    fn readU32(*_): u32? {
        if (_.data.len - _.pos < 4) return null;
        const v: u32 = (_.data[_.pos] as u32)
            | ((_.data[_.pos + 1] as u32) << 8)
            | ((_.data[_.pos + 2] as u32) << 16)
            | ((_.data[_.pos + 3] as u32) << 24);
        _.pos += 4;
        return v;
    }

    fn readI32(*_): i32? {
        const v = _.readU32() ?? return null;
        return v as i32;
    }

    fn readU64(*_): u64? {
        if (_.data.len - _.pos < 8) return null;
        let acc: u64 = 0;
        let i: usize = 0;
        while (i < 8) {
            const shift: u6 = (i * 8) as u6;
            acc |= (_.data[_.pos + i] as u64) << shift;
            i += 1;
        }
        _.pos += 8;
        return acc;
    }

    fn readI64(*_): i64? {
        const v = _.readU64() ?? return null;
        return v as i64;
    }

    /// 严格规范性解码 `Compact<u64>`（拒绝所有非最小编码与大整数尾零）
    fn readCompactU64(*_): u64? {
        if (_.data.len - _.pos < 1) return null;
        const b0 = _.data[_.pos];
        const mode = b0 & 0x03;

        if (mode == 0) {
            _.pos += 1;
            return (b0 >> 2) as u64;
        }

        if (mode == 1) {
            if (_.data.len - _.pos < 2) return null;
            const raw: u16 = (b0 as u16) | ((_.data[_.pos + 1] as u16) << 8);
            const v: u64 = (raw >> 2) as u64;
            if (v < 64) return null;
            _.pos += 2;
            return v;
        }

        if (mode == 2) {
            if (_.data.len - _.pos < 4) return null;
            const raw: u32 = (b0 as u32)
                | ((_.data[_.pos + 1] as u32) << 8)
                | ((_.data[_.pos + 2] as u32) << 16)
                | ((_.data[_.pos + 3] as u32) << 24);
            const v: u64 = (raw >> 2) as u64;
            if (v < 16384) return null;
            _.pos += 4;
            return v;
        }

        // mode == 3: 大整数模式，n = (b0 >> 2) + 4 字节小端
        const n: usize = ((b0 >> 2) as usize) + 4;
        if (n > 8) return null;
        if (_.data.len - _.pos < n + 1) return null;
        // 严格规范性：最高位字节（小端末尾字节）不得为 0x00
        if (_.data[_.pos + n] == 0x00) return null;

        let acc: u64 = 0;
        let i: usize = 0;
        while (i < n) {
            const shift: u6 = (i * 8) as u6;
            acc |= (_.data[_.pos + 1 + i] as u64) << shift;
            i += 1;
        }
        if (acc < 1073741824) return null;
        _.pos += n + 1;
        return acc;
    }

    /// 零拷贝读取带 `Compact<u64>` 长度前缀的字节切片
    fn readBytes(*_): ([]const u8)? {
        const saved = _.pos;
        const len_u64 = _.readCompactU64() ?? return null;
        const len: usize = len_u64 as usize;
        if (len > _.data.len - _.pos) {
            _.pos = saved;
            return null;
        }
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    /// 零拷贝读取带 `Compact<u64>` 长度前缀的字符串切片
    fn readString(*_): ([]const u8)? {
        return _.readBytes();
    }
}
