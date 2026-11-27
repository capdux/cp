// Origin: Pure-CP Stdlib (Zero-Alloc Borsh Deterministic Binary Codec) | Created: 2026-11-22

fn isF32NanBits(bits: u32): bool {
    return (bits & 0x7F80_0000) == 0x7F80_0000 && (bits & 0x007F_FFFF) != 0;
}

fn isF64NanBits(bits: u64): bool {
    return (bits & 0x7FF0_0000_0000_0000) == 0x7FF0_0000_0000_0000 &&
        (bits & 0x000F_FFFF_FFFF_FFFF) != 0;
}

pub struct BorshWriter {
    buf: []u8,
    pos: usize,

    static fn init(buf: []u8): BorshWriter {
        return BorshWriter{
            .buf = buf,
            .pos = 0,
        };
    }

    fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    fn remaining(): usize {
        if (_.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    fn writeU8(*_, v: u8): bool {
        if (_.remaining() < 1) {
            return false;
        }
        _.buf[_.pos] = v;
        _.pos += 1;
        return true;
    }

    fn writeI8(*_, v: i8): bool {
        let bits: u8 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        return _.writeU8(bits);
    }

    fn writeBool(*_, v: bool): bool {
        if (v) {
            return _.writeU8(1);
        }
        return _.writeU8(0);
    }

    fn writeU16(*_, v: u16): bool {
        if (_.remaining() < 2) {
            return false;
        }
        _.buf[_.pos] = (v & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.pos += 2;
        return true;
    }

    fn writeI16(*_, v: i16): bool {
        let bits: u16 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        return _.writeU16(bits);
    }

    fn writeU32(*_, v: u32): bool {
        if (_.remaining() < 4) {
            return false;
        }
        _.buf[_.pos] = (v & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        _.pos += 4;
        return true;
    }

    fn writeI32(*_, v: i32): bool {
        let bits: u32 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        return _.writeU32(bits);
    }

    fn writeU64(*_, v: u64): bool {
        if (_.remaining() < 8) {
            return false;
        }
        _.buf[_.pos] = (v & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        _.buf[_.pos + 4] = ((v >> 32) & 0xFF) as u8;
        _.buf[_.pos + 5] = ((v >> 40) & 0xFF) as u8;
        _.buf[_.pos + 6] = ((v >> 48) & 0xFF) as u8;
        _.buf[_.pos + 7] = ((v >> 56) & 0xFF) as u8;
        _.pos += 8;
        return true;
    }

    fn writeI64(*_, v: i64): bool {
        let bits: u64 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        return _.writeU64(bits);
    }

    fn writeF32(*_, v: f32): bool {
        let bits: u32 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        if (isF32NanBits(bits)) {
            return false;
        }
        return _.writeU32(bits);
    }

    fn writeF64(*_, v: f64): bool {
        let bits: u64 = 0;
        unsafe {
            bits = @bitCast(v);
        }
        if (isF64NanBits(bits)) {
            return false;
        }
        return _.writeU64(bits);
    }

    fn writeFixedBytes(*_, data: []const u8): bool {
        if (data.len > _.remaining()) {
            return false;
        }
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.pos + i] = data[i];
            i += 1;
        }
        _.pos += data.len;
        return true;
    }

    fn writeBytes(*_, data: []const u8): bool {
        if (data.len > 0xFFFF_FFFF) {
            return false;
        }
        if (_.remaining() < 4 || data.len > _.remaining() - 4) {
            return false;
        }
        if (!_.writeU32(data.len as u32)) {
            return false;
        }
        return _.writeFixedBytes(data);
    }

    fn writeString(*_, s: []const u8): bool {
        return _.writeBytes(s);
    }

    fn writeOptionTag(*_, is_some: bool): bool {
        return _.writeBool(is_some);
    }

    fn writeEnumTag(*_, variant: u8): bool {
        return _.writeU8(variant);
    }

    fn writeSeqLen(*_, count: u32): bool {
        return _.writeU32(count);
    }
}

pub struct BorshReader {
    data: []const u8,
    pos: usize,

    static fn init(data: []const u8): BorshReader {
        return BorshReader{
            .data = data,
            .pos = 0,
        };
    }

    fn remaining(): usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    fn isEof(): bool {
        return _.pos >= _.data.len;
    }

    fn readU8(*_): u8? {
        if (_.remaining() < 1) {
            return null;
        }
        const v = _.data[_.pos];
        _.pos += 1;
        return v;
    }

    fn readI8(*_): i8? {
        const raw = _.readU8() ?? return null;
        unsafe {
            const v: i8 = @bitCast(raw);
            return v;
        }
    }

    fn readBool(*_): bool? {
        const raw = _.readU8() ?? return null;
        if (raw == 0) return false;
        if (raw == 1) return true;
        return null;
    }

    fn readU16(*_): u16? {
        if (_.remaining() < 2) {
            return null;
        }
        const b0 = _.data[_.pos] as u16;
        const b1 = _.data[_.pos + 1] as u16;
        _.pos += 2;
        return b0 | (b1 << 8);
    }

    fn readI16(*_): i16? {
        const raw = _.readU16() ?? return null;
        unsafe {
            const v: i16 = @bitCast(raw);
            return v;
        }
    }

    fn readU32(*_): u32? {
        if (_.remaining() < 4) {
            return null;
        }
        const b0 = _.data[_.pos] as u32;
        const b1 = _.data[_.pos + 1] as u32;
        const b2 = _.data[_.pos + 2] as u32;
        const b3 = _.data[_.pos + 3] as u32;
        _.pos += 4;
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }

    fn readI32(*_): i32? {
        const raw = _.readU32() ?? return null;
        unsafe {
            const v: i32 = @bitCast(raw);
            return v;
        }
    }

    fn readU64(*_): u64? {
        if (_.remaining() < 8) {
            return null;
        }
        const b0 = _.data[_.pos] as u64;
        const b1 = _.data[_.pos + 1] as u64;
        const b2 = _.data[_.pos + 2] as u64;
        const b3 = _.data[_.pos + 3] as u64;
        const b4 = _.data[_.pos + 4] as u64;
        const b5 = _.data[_.pos + 5] as u64;
        const b6 = _.data[_.pos + 6] as u64;
        const b7 = _.data[_.pos + 7] as u64;
        _.pos += 8;
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24) |
            (b4 << 32) | (b5 << 40) | (b6 << 48) | (b7 << 56);
    }

    fn readI64(*_): i64? {
        const raw = _.readU64() ?? return null;
        unsafe {
            const v: i64 = @bitCast(raw);
            return v;
        }
    }

    fn readF32(*_): f32? {
        const bits = _.readU32() ?? return null;
        if (isF32NanBits(bits)) {
            return null;
        }
        unsafe {
            const v: f32 = @bitCast(bits);
            return v;
        }
    }

    fn readF64(*_): f64? {
        const bits = _.readU64() ?? return null;
        if (isF64NanBits(bits)) {
            return null;
        }
        unsafe {
            const v: f64 = @bitCast(bits);
            return v;
        }
    }

    fn readFixedBytes(*_, n: usize): ([]const u8)? {
        if (n > _.remaining()) {
            return null;
        }
        const start = _.pos;
        _.pos += n;
        return _.data[start.._.pos];
    }

    fn readBytes(*_): ([]const u8)? {
        const len_u32 = _.readU32() ?? return null;
        const n: usize = len_u32 as usize;
        return _.readFixedBytes(n);
    }

    fn readString(*_): ([]const u8)? {
        return _.readBytes();
    }

    fn readOptionTag(*_): bool? {
        return _.readBool();
    }

    fn readEnumTag(*_): u8? {
        return _.readU8();
    }

    fn readSeqLen(*_): u32? {
        return _.readU32();
    }
}
