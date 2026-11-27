// Origin: Pure-CP Stdlib (Zero-Alloc Named Binary Tag Parser: Java BE & Bedrock LE) | Created: 2026-11-22

pub const MAX_NBT_DEPTH: u32 = 64;

pub enum NbtTagType: u8 {
    end = 0,
    byte = 1,
    short = 2,
    int = 3,
    long = 4,
    float = 5,
    double = 6,
    byte_array = 7,
    string = 8,
    list = 9,
    compound = 10,
    int_array = 11,
    long_array = 12,
}

pub enum NbtEndian: u8 {
    big = 0,
    little = 1,
}

pub fn nbtTagTypeFromByte(raw: u8): NbtTagType? {
    if (raw == 0) return NbtTagType.end;
    if (raw == 1) return NbtTagType.byte;
    if (raw == 2) return NbtTagType.short;
    if (raw == 3) return NbtTagType.int;
    if (raw == 4) return NbtTagType.long;
    if (raw == 5) return NbtTagType.float;
    if (raw == 6) return NbtTagType.double;
    if (raw == 7) return NbtTagType.byte_array;
    if (raw == 8) return NbtTagType.string;
    if (raw == 9) return NbtTagType.list;
    if (raw == 10) return NbtTagType.compound;
    if (raw == 11) return NbtTagType.int_array;
    if (raw == 12) return NbtTagType.long_array;
    return null;
}

pub struct NbtTagHeader {
    tag_type: NbtTagType,
    name: []const u8,
}

pub struct NbtListHeader {
    elem_type: NbtTagType,
    count: u32,
}

pub struct NbtFieldRef {
    tag_type: NbtTagType,
    name: []const u8,
    payload_offset: usize,
    payload_end: usize,
    payload: []const u8,
}

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub struct NbtReader {
    data: []const u8,
    pos: usize,
    endian: NbtEndian,

    static fn init(data: []const u8): NbtReader {
        return NbtReader{
            .data = data,
            .pos = 0,
            .endian = NbtEndian.big,
        };
    }

    static fn initLittle(data: []const u8): NbtReader {
        return NbtReader{
            .data = data,
            .pos = 0,
            .endian = NbtEndian.little,
        };
    }

    fn remaining(): usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    fn skipBytes(*_, n: usize): bool {
        if (n > _.remaining()) {
            return false;
        }
        _.pos += n;
        return true;
    }

    fn readU8(*_): u8? {
        if (_.remaining() < 1) {
            return null;
        }
        const v = _.data[_.pos];
        _.pos += 1;
        return v;
    }

    fn readU16(*_): u16? {
        if (_.remaining() < 2) {
            return null;
        }
        const b0 = _.data[_.pos] as u16;
        const b1 = _.data[_.pos + 1] as u16;
        _.pos += 2;
        if (_.endian == NbtEndian.big) {
            return (b0 << 8) | b1;
        }
        return b0 | (b1 << 8);
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
        if (_.endian == NbtEndian.big) {
            return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
        }
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
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
        if (_.endian == NbtEndian.big) {
            return (b0 << 56) | (b1 << 48) | (b2 << 40) | (b3 << 32) |
                (b4 << 24) | (b5 << 16) | (b6 << 8) | b7;
        }
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24) |
            (b4 << 32) | (b5 << 40) | (b6 << 48) | (b7 << 56);
    }

    fn readByte(*_): i8? {
        const raw = _.readU8() ?? return null;
        unsafe {
            const v: i8 = @bitCast(raw);
            return v;
        }
    }

    fn readShort(*_): i16? {
        const raw = _.readU16() ?? return null;
        unsafe {
            const v: i16 = @bitCast(raw);
            return v;
        }
    }

    fn readInt(*_): i32? {
        const raw = _.readU32() ?? return null;
        unsafe {
            const v: i32 = @bitCast(raw);
            return v;
        }
    }

    fn readLong(*_): i64? {
        const raw = _.readU64() ?? return null;
        unsafe {
            const v: i64 = @bitCast(raw);
            return v;
        }
    }

    fn readFloat(*_): f32? {
        const raw = _.readU32() ?? return null;
        unsafe {
            const v: f32 = @bitCast(raw);
            return v;
        }
    }

    fn readDouble(*_): f64? {
        const raw = _.readU64() ?? return null;
        unsafe {
            const v: f64 = @bitCast(raw);
            return v;
        }
    }

    fn readString(*_): ([]const u8)? {
        const len_u16 = _.readU16() ?? return null;
        const len: usize = len_u16 as usize;
        if (len > _.remaining()) {
            return null;
        }
        const start = _.pos;
        _.pos += len;
        return _.data[start.._.pos];
    }

    fn readByteArray(*_): ([]const u8)? {
        const count_i32 = _.readInt() ?? return null;
        if (count_i32 < 0) {
            return null;
        }
        const count: usize = count_i32 as usize;
        if (count > _.remaining()) {
            return null;
        }
        const start = _.pos;
        _.pos += count;
        return _.data[start.._.pos];
    }

    fn readIntArrayRaw(*_): ([]const u8)? {
        const count_i32 = _.readInt() ?? return null;
        if (count_i32 < 0) {
            return null;
        }
        const count: usize = count_i32 as usize;
        if (count > _.remaining() / 4) {
            return null;
        }
        const byte_len = count * 4;
        const start = _.pos;
        _.pos += byte_len;
        return _.data[start.._.pos];
    }

    fn readLongArrayRaw(*_): ([]const u8)? {
        const count_i32 = _.readInt() ?? return null;
        if (count_i32 < 0) {
            return null;
        }
        const count: usize = count_i32 as usize;
        if (count > _.remaining() / 8) {
            return null;
        }
        const byte_len = count * 8;
        const start = _.pos;
        _.pos += byte_len;
        return _.data[start.._.pos];
    }

    fn readListHeader(*_): NbtListHeader? {
        const raw_elem = _.readU8() ?? return null;
        const elem_type = nbtTagTypeFromByte(raw_elem) ?? return null;
        const count_i32 = _.readInt() ?? return null;
        if (count_i32 < 0) {
            return null;
        }
        const count: u32 = count_i32 as u32;
        if (count > 0 && elem_type == NbtTagType.end) {
            return null;
        }
        return NbtListHeader{
            .elem_type = elem_type,
            .count = count,
        };
    }

    fn readRootHeader(*_): NbtTagHeader? {
        const raw_tag = _.readU8() ?? return null;
        const tag_type = nbtTagTypeFromByte(raw_tag) ?? return null;
        if (tag_type != NbtTagType.compound) {
            return null;
        }
        const name = _.readString() ?? return null;
        return NbtTagHeader{
            .tag_type = tag_type,
            .name = name,
        };
    }

    fn nextCompoundEntry(*_): NbtTagHeader? {
        const raw_tag = _.readU8() ?? return null;
        const tag_type = nbtTagTypeFromByte(raw_tag) ?? return null;
        if (tag_type == NbtTagType.end) {
            return null;
        }
        const name = _.readString() ?? return null;
        return NbtTagHeader{
            .tag_type = tag_type,
            .name = name,
        };
    }

    fn skipPayloadDepth(*_, tag_type: NbtTagType, depth: u32): bool {
        if (depth > MAX_NBT_DEPTH) {
            return false;
        }
        if (tag_type == NbtTagType.end) {
            return true;
        }
        if (tag_type == NbtTagType.byte) {
            return _.skipBytes(1);
        }
        if (tag_type == NbtTagType.short) {
            return _.skipBytes(2);
        }
        if (tag_type == NbtTagType.int || tag_type == NbtTagType.float) {
            return _.skipBytes(4);
        }
        if (tag_type == NbtTagType.long || tag_type == NbtTagType.double) {
            return _.skipBytes(8);
        }
        if (tag_type == NbtTagType.string) {
            if (_s := _.readString()) {
                return true;
            }
            return false;
        }
        if (tag_type == NbtTagType.byte_array) {
            if (_a := _.readByteArray()) {
                return true;
            }
            return false;
        }
        if (tag_type == NbtTagType.int_array) {
            if (_a := _.readIntArrayRaw()) {
                return true;
            }
            return false;
        }
        if (tag_type == NbtTagType.long_array) {
            if (_a := _.readLongArrayRaw()) {
                return true;
            }
            return false;
        }
        if (tag_type == NbtTagType.list) {
            if (depth + 1 > MAX_NBT_DEPTH) {
                return false;
            }
            const lh = _.readListHeader() ?? return false;
            let i: u32 = 0;
            while (i < lh.count) {
                if (!_.skipPayloadDepth(lh.elem_type, depth + 1)) {
                    return false;
                }
                i += 1;
            }
            return true;
        }
        if (tag_type == NbtTagType.compound) {
            if (depth + 1 > MAX_NBT_DEPTH) {
                return false;
            }
            while (true) {
                const raw_tag = _.readU8() ?? return false;
                const sub_type = nbtTagTypeFromByte(raw_tag) ?? return false;
                if (sub_type == NbtTagType.end) {
                    return true;
                }
                if (_nm := _.readString()) {
                    // valid entry name
                } else {
                    return false;
                }
                if (!_.skipPayloadDepth(sub_type, depth + 1)) {
                    return false;
                }
            }
        }
        return false;
    }

    fn skipPayload(*_, tag_type: NbtTagType): bool {
        return _.skipPayloadDepth(tag_type, 0);
    }
}

pub fn isNbtCompound(data: []const u8): bool {
    if (data.len < 3) {
        return false;
    }
    if (data[0] != 10) {
        return false;
    }
    const name_len: usize = (((data[1] as u16) << 8) | (data[2] as u16)) as usize;
    return name_len <= data.len - 3;
}

pub fn findNbtFieldEndian(data: []const u8, target_name: []const u8, endian: NbtEndian): NbtFieldRef? {
    let reader = NbtReader{
        .data = data,
        .pos = 0,
        .endian = endian,
    };
    if (_root := reader.readRootHeader()) {
        // root compound valid
    } else {
        return null;
    }

    while (true) {
        const raw_tag = reader.readU8() ?? return null;
        const tag_type = nbtTagTypeFromByte(raw_tag) ?? return null;
        if (tag_type == NbtTagType.end) {
            return null;
        }
        const name = reader.readString() ?? return null;
        const payload_offset = reader.pos;
        if (!reader.skipPayload(tag_type)) {
            return null;
        }
        const payload_end = reader.pos;
        if (sliceEq(name, target_name)) {
            return NbtFieldRef{
                .tag_type = tag_type,
                .name = name,
                .payload_offset = payload_offset,
                .payload_end = payload_end,
                .payload = data[payload_offset..payload_end],
            };
        }
    }
}

pub fn findNbtField(data: []const u8, target_name: []const u8): NbtFieldRef? {
    return findNbtFieldEndian(data, target_name, NbtEndian.big);
}
