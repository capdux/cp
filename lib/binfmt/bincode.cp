// Origin: Pure-CP Stdlib (Zero-Alloc Bincode v1/v2 Binary Codec) | Created: 2026-11-22

pub const BINCODE_VARINT_U16_TAG: u8 = 251;
pub const BINCODE_VARINT_U32_TAG: u8 = 252;
pub const BINCODE_VARINT_U64_TAG: u8 = 253;

pub enum BincodeEndian: u8 {
    little = 0,
    big = 1,
}

pub enum BincodeIntEncoding: u8 {
    fixed = 0,
    varint = 1,
}

pub struct BincodeConfig {
    endian: BincodeEndian,
    int_encoding: BincodeIntEncoding,

    fn legacy(): BincodeConfig {
        return BincodeConfig{
            .endian = BincodeEndian.little,
            .int_encoding = BincodeIntEncoding.fixed,
        };
    }

    fn standard(): BincodeConfig {
        return BincodeConfig{
            .endian = BincodeEndian.little,
            .int_encoding = BincodeIntEncoding.varint,
        };
    }

    fn bigEndianFixed(): BincodeConfig {
        return BincodeConfig{
            .endian = BincodeEndian.big,
            .int_encoding = BincodeIntEncoding.fixed,
        };
    }
}

fn zzEncI16(v: i16): u16 {
    let u: u16 = 0;
    unsafe { u = @bitCast(v); }
    let mask: u16 = 0;
    if (v < 0) mask = 0xFFFF;
    return (u << 1) ^ mask;
}

fn zzDecI16(z: u16): i16 {
    let mask: u16 = 0;
    if ((z & 1) != 0) mask = 0xFFFF;
    const raw: u16 = (z >> 1) ^ mask;
    unsafe {
        const v: i16 = @bitCast(raw);
        return v;
    }
}

fn zzEncI32(v: i32): u32 {
    let u: u32 = 0;
    unsafe { u = @bitCast(v); }
    let mask: u32 = 0;
    if (v < 0) mask = 0xFFFF_FFFF;
    return (u << 1) ^ mask;
}

fn zzDecI32(z: u32): i32 {
    let mask: u32 = 0;
    if ((z & 1) != 0) mask = 0xFFFF_FFFF;
    const raw: u32 = (z >> 1) ^ mask;
    unsafe {
        const v: i32 = @bitCast(raw);
        return v;
    }
}

fn zzEncI64(v: i64): u64 {
    let u: u64 = 0;
    unsafe { u = @bitCast(v); }
    let mask: u64 = 0;
    if (v < 0) mask = 0xFFFF_FFFF_FFFF_FFFF;
    return (u << 1) ^ mask;
}

fn zzDecI64(z: u64): i64 {
    let mask: u64 = 0;
    if ((z & 1) != 0) mask = 0xFFFF_FFFF_FFFF_FFFF;
    const raw: u64 = (z >> 1) ^ mask;
    unsafe {
        const v: i64 = @bitCast(raw);
        return v;
    }
}

pub struct BincodeWriter {
    buf: []u8,
    pos: usize,
    cfg: BincodeConfig,

    fn init(buf: []u8, cfg: BincodeConfig): BincodeWriter {
        return BincodeWriter{
            .buf = buf,
            .pos = 0,
            .cfg = cfg,
        };
    }

    static fn initStandard(buf: []u8): BincodeWriter {
        return BincodeWriter.init(buf, BincodeConfig.standard());
    }

    static fn initLegacy(buf: []u8): BincodeWriter {
        return BincodeWriter.init(buf, BincodeConfig.legacy());
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

    fn writeFixedU16(*_, v: u16): bool {
        if (_.remaining() < 2) return false;
        if (_.cfg.endian == BincodeEndian.little) {
            _.buf[_.pos] = (v & 0xFF) as u8;
            _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        } else {
            _.buf[_.pos] = ((v >> 8) & 0xFF) as u8;
            _.buf[_.pos + 1] = (v & 0xFF) as u8;
        }
        _.pos += 2;
        return true;
    }

    fn writeFixedU32(*_, v: u32): bool {
        if (_.remaining() < 4) return false;
        if (_.cfg.endian == BincodeEndian.little) {
            _.buf[_.pos] = (v & 0xFF) as u8;
            _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
            _.buf[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
            _.buf[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        } else {
            _.buf[_.pos] = ((v >> 24) & 0xFF) as u8;
            _.buf[_.pos + 1] = ((v >> 16) & 0xFF) as u8;
            _.buf[_.pos + 2] = ((v >> 8) & 0xFF) as u8;
            _.buf[_.pos + 3] = (v & 0xFF) as u8;
        }
        _.pos += 4;
        return true;
    }

    fn writeFixedU64(*_, v: u64): bool {
        if (_.remaining() < 8) return false;
        if (_.cfg.endian == BincodeEndian.little) {
            _.buf[_.pos] = (v & 0xFF) as u8;
            _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
            _.buf[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
            _.buf[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
            _.buf[_.pos + 4] = ((v >> 32) & 0xFF) as u8;
            _.buf[_.pos + 5] = ((v >> 40) & 0xFF) as u8;
            _.buf[_.pos + 6] = ((v >> 48) & 0xFF) as u8;
            _.buf[_.pos + 7] = ((v >> 56) & 0xFF) as u8;
        } else {
            _.buf[_.pos] = ((v >> 56) & 0xFF) as u8;
            _.buf[_.pos + 1] = ((v >> 48) & 0xFF) as u8;
            _.buf[_.pos + 2] = ((v >> 40) & 0xFF) as u8;
            _.buf[_.pos + 3] = ((v >> 32) & 0xFF) as u8;
            _.buf[_.pos + 4] = ((v >> 24) & 0xFF) as u8;
            _.buf[_.pos + 5] = ((v >> 16) & 0xFF) as u8;
            _.buf[_.pos + 6] = ((v >> 8) & 0xFF) as u8;
            _.buf[_.pos + 7] = (v & 0xFF) as u8;
        }
        _.pos += 8;
        return true;
    }

    fn writeVarUint(*_, v: u64): bool {
        if (v <= 250) {
            return _.writeU8(v as u8);
        }
        if (v <= 65535) {
            if (_.remaining() < 3) return false;
            _.buf[_.pos] = BINCODE_VARINT_U16_TAG;
            _.pos += 1;
            return _.writeFixedU16(v as u16);
        }
        if (v <= 0xFFFF_FFFF) {
            if (_.remaining() < 5) return false;
            _.buf[_.pos] = BINCODE_VARINT_U32_TAG;
            _.pos += 1;
            return _.writeFixedU32(v as u32);
        }
        if (_.remaining() < 9) return false;
        _.buf[_.pos] = BINCODE_VARINT_U64_TAG;
        _.pos += 1;
        return _.writeFixedU64(v);
    }

    fn writeU8(*_, v: u8): bool {
        if (_.remaining() < 1) return false;
        _.buf[_.pos] = v;
        _.pos += 1;
        return true;
    }

    fn writeI8(*_, v: i8): bool {
        let bits: u8 = 0;
        unsafe { bits = @bitCast(v); }
        return _.writeU8(bits);
    }

    fn writeBool(*_, v: bool): bool {
        if (v) return _.writeU8(1);
        return _.writeU8(0);
    }

    fn writeU16(*_, v: u16): bool {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            return _.writeVarUint(v as u64);
        }
        return _.writeFixedU16(v);
    }

    fn writeI16(*_, v: i16): bool {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            return _.writeVarUint(zzEncI16(v) as u64);
        }
        let bits: u16 = 0;
        unsafe { bits = @bitCast(v); }
        return _.writeFixedU16(bits);
    }

    fn writeU32(*_, v: u32): bool {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            return _.writeVarUint(v as u64);
        }
        return _.writeFixedU32(v);
    }

    fn writeI32(*_, v: i32): bool {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            return _.writeVarUint(zzEncI32(v) as u64);
        }
        let bits: u32 = 0;
        unsafe { bits = @bitCast(v); }
        return _.writeFixedU32(bits);
    }

    fn writeU64(*_, v: u64): bool {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            return _.writeVarUint(v);
        }
        return _.writeFixedU64(v);
    }

    fn writeI64(*_, v: i64): bool {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            return _.writeVarUint(zzEncI64(v));
        }
        let bits: u64 = 0;
        unsafe { bits = @bitCast(v); }
        return _.writeFixedU64(bits);
    }

    fn writeF32(*_, v: f32): bool {
        let bits: u32 = 0;
        unsafe { bits = @bitCast(v); }
        return _.writeFixedU32(bits);
    }

    fn writeF64(*_, v: f64): bool {
        let bits: u64 = 0;
        unsafe { bits = @bitCast(v); }
        return _.writeFixedU64(bits);
    }

    fn writeBytes(*_, data: []const u8): bool {
        const orig = _.pos;
        if (!_.writeU64(data.len as u64)) {
            _.pos = orig;
            return false;
        }
        if (data.len > _.remaining()) {
            _.pos = orig;
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

    fn writeString(*_, s: []const u8): bool {
        return _.writeBytes(s);
    }

    fn writeOptionTag(*_, is_some: bool): bool {
        return _.writeBool(is_some);
    }

    fn writeVariantIdx(*_, idx: u32): bool {
        return _.writeU32(idx);
    }

    fn writeSeqLen(*_, len: u64): bool {
        return _.writeU64(len);
    }
}

pub struct BincodeReader {
    data: []const u8,
    pos: usize,
    cfg: BincodeConfig,

    fn init(data: []const u8, cfg: BincodeConfig): BincodeReader {
        return BincodeReader{
            .data = data,
            .pos = 0,
            .cfg = cfg,
        };
    }

    static fn initStandard(data: []const u8): BincodeReader {
        return BincodeReader.init(data, BincodeConfig.standard());
    }

    static fn initLegacy(data: []const u8): BincodeReader {
        return BincodeReader.init(data, BincodeConfig.legacy());
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

    fn readFixedU16(*_): u16? {
        if (_.remaining() < 2) return null;
        const b0 = _.data[_.pos] as u16;
        const b1 = _.data[_.pos + 1] as u16;
        _.pos += 2;
        if (_.cfg.endian == BincodeEndian.little) {
            return b0 | (b1 << 8);
        }
        return (b0 << 8) | b1;
    }

    fn readFixedU32(*_): u32? {
        if (_.remaining() < 4) return null;
        const b0 = _.data[_.pos] as u32;
        const b1 = _.data[_.pos + 1] as u32;
        const b2 = _.data[_.pos + 2] as u32;
        const b3 = _.data[_.pos + 3] as u32;
        _.pos += 4;
        if (_.cfg.endian == BincodeEndian.little) {
            return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
        }
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }

    fn readFixedU64(*_): u64? {
        if (_.remaining() < 8) return null;
        const b0 = _.data[_.pos] as u64;
        const b1 = _.data[_.pos + 1] as u64;
        const b2 = _.data[_.pos + 2] as u64;
        const b3 = _.data[_.pos + 3] as u64;
        const b4 = _.data[_.pos + 4] as u64;
        const b5 = _.data[_.pos + 5] as u64;
        const b6 = _.data[_.pos + 6] as u64;
        const b7 = _.data[_.pos + 7] as u64;
        _.pos += 8;
        if (_.cfg.endian == BincodeEndian.little) {
            return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24) |
                (b4 << 32) | (b5 << 40) | (b6 << 48) | (b7 << 56);
        }
        return (b0 << 56) | (b1 << 48) | (b2 << 40) | (b3 << 32) |
            (b4 << 24) | (b5 << 16) | (b6 << 8) | b7;
    }

    fn readVarUint(*_): u64? {
        const tag = _.readU8() ?? return null;
        if (tag <= 250) {
            return tag as u64;
        }
        if (tag == BINCODE_VARINT_U16_TAG) {
            const v = _.readFixedU16() ?? return null;
            return v as u64;
        }
        if (tag == BINCODE_VARINT_U32_TAG) {
            const v = _.readFixedU32() ?? return null;
            return v as u64;
        }
        if (tag == BINCODE_VARINT_U64_TAG) {
            return _.readFixedU64();
        }
        // Tags 254 and 255 are invalid for <= 64-bit integers
        return null;
    }

    fn readU8(*_): u8? {
        if (_.remaining() < 1) return null;
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
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            const v = _.readVarUint() ?? return null;
            if (v > 65535) return null;
            return v as u16;
        }
        return _.readFixedU16();
    }

    fn readI16(*_): i16? {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            const v = _.readVarUint() ?? return null;
            if (v > 65535) return null;
            return zzDecI16(v as u16);
        }
        const raw = _.readFixedU16() ?? return null;
        unsafe {
            const v: i16 = @bitCast(raw);
            return v;
        }
    }

    fn readU32(*_): u32? {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            const v = _.readVarUint() ?? return null;
            if (v > 0xFFFF_FFFF) return null;
            return v as u32;
        }
        return _.readFixedU32();
    }

    fn readI32(*_): i32? {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            const v = _.readVarUint() ?? return null;
            if (v > 0xFFFF_FFFF) return null;
            return zzDecI32(v as u32);
        }
        const raw = _.readFixedU32() ?? return null;
        unsafe {
            const v: i32 = @bitCast(raw);
            return v;
        }
    }

    fn readU64(*_): u64? {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            return _.readVarUint();
        }
        return _.readFixedU64();
    }

    fn readI64(*_): i64? {
        if (_.cfg.int_encoding == BincodeIntEncoding.varint) {
            const v = _.readVarUint() ?? return null;
            return zzDecI64(v);
        }
        const raw = _.readFixedU64() ?? return null;
        unsafe {
            const v: i64 = @bitCast(raw);
            return v;
        }
    }

    fn readF32(*_): f32? {
        const raw = _.readFixedU32() ?? return null;
        unsafe {
            const v: f32 = @bitCast(raw);
            return v;
        }
    }

    fn readF64(*_): f64? {
        const raw = _.readFixedU64() ?? return null;
        unsafe {
            const v: f64 = @bitCast(raw);
            return v;
        }
    }

    fn readBytes(*_): ([]const u8)? {
        const len_u64 = _.readU64() ?? return null;
        if (len_u64 > (_.remaining() as u64)) {
            return null;
        }
        const n = len_u64 as usize;
        const start = _.pos;
        _.pos += n;
        return _.data[start.._.pos];
    }

    fn readString(*_): ([]const u8)? {
        return _.readBytes();
    }

    fn readOptionTag(*_): bool? {
        return _.readBool();
    }

    fn readVariantIdx(*_): u32? {
        return _.readU32();
    }

    fn readSeqLen(*_): u64? {
        return _.readU64();
    }
}
