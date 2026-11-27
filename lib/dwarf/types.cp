// Source: cp/lib/dwarf/types.cp (DWARF v4/v5 shared types, constants, and binary cursor)

const std = @import("std");

pub const DwarfError = error{
    TruncatedData,
    UnsupportedVersion,
    InvalidHeader,
    InvalidOpcode,
    InvalidFileIndex,
    MissingAbbrev,
    UnsupportedForm,
    InvalidUtf8,
    LoopLimitExceeded,
    InvalidLeb128,
    OutOfMemory,
};

pub struct DwTag {
    pub const array_type: u32 = 0x01;
    pub const class_type: u32 = 0x02;
    pub const entry_point: u32 = 0x03;
    pub const enumeration_type: u32 = 0x04;
    pub const formal_parameter: u32 = 0x05;
    pub const lexical_block: u32 = 0x0b;
    pub const member: u32 = 0x0d;
    pub const pointer_type: u32 = 0x0f;
    pub const compile_unit: u32 = 0x11;
    pub const structure_type: u32 = 0x13;
    pub const typedef_type: u32 = 0x16;
    pub const inlined_subroutine: u32 = 0x1d;
    pub const base_type: u32 = 0x24;
    pub const subprogram: u32 = 0x2e;
    pub const variable: u32 = 0x34;
    pub const namespace_tag: u32 = 0x39;
}

pub struct DwAt {
    pub const location: u32 = 0x02;
    pub const name: u32 = 0x03;
    pub const byte_size: u32 = 0x0b;
    pub const stmt_list: u32 = 0x10;
    pub const low_pc: u32 = 0x11;
    pub const high_pc: u32 = 0x12;
    pub const language: u32 = 0x13;
    pub const comp_dir: u32 = 0x1b;
    pub const producer: u32 = 0x25;
    pub const decl_file: u32 = 0x3a;
    pub const decl_line: u32 = 0x3b;
    pub const external: u32 = 0x3f;
    pub const type_attr: u32 = 0x49;
    pub const linkage_name: u32 = 0x6e;
}

pub struct DwForm {
    pub const addr: u32 = 0x01;
    pub const block2: u32 = 0x03;
    pub const block4: u32 = 0x04;
    pub const data2: u32 = 0x05;
    pub const data4: u32 = 0x06;
    pub const data8: u32 = 0x07;
    pub const string: u32 = 0x08;
    pub const block: u32 = 0x09;
    pub const block1: u32 = 0x0a;
    pub const data1: u32 = 0x0b;
    pub const flag: u32 = 0x0c;
    pub const sdata: u32 = 0x0d;
    pub const strp: u32 = 0x0e;
    pub const udata: u32 = 0x0f;
    pub const ref_addr: u32 = 0x10;
    pub const ref1: u32 = 0x11;
    pub const ref2: u32 = 0x12;
    pub const ref4: u32 = 0x13;
    pub const ref8: u32 = 0x14;
    pub const ref_udata: u32 = 0x15;
    pub const indirect: u32 = 0x16;
    pub const sec_offset: u32 = 0x17;
    pub const exprloc: u32 = 0x18;
    pub const flag_present: u32 = 0x19;
    pub const line_strp: u32 = 0x1f;
}

pub struct Cursor {
    data: []const u8,
    pos: usize,

    pub static fn init(data: []const u8): Cursor {
        return Cursor{ .data = data, .pos = 0 };
    }

    pub fn remaining(): usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    pub fn readU8(*_): !u8 {
        if (_.pos >= _.data.len) {
            return DwarfError.TruncatedData;
        }
        const b = _.data[_.pos];
        _.pos += 1;
        return b;
    }

    pub fn readI8(*_): !i8 {
        const b = try _.readU8();
        return @bitCast(b);
    }

    pub fn readU16(*_): !u16 {
        if (_.remaining() < 2) {
            return DwarfError.TruncatedData;
        }
        const b0: u16 = @intCast(_.data[_.pos]);
        const b1: u16 = @intCast(_.data[_.pos + 1]);
        _.pos += 2;
        return b0 | (b1 << 8);
    }

    pub fn readU32(*_): !u32 {
        if (_.remaining() < 4) {
            return DwarfError.TruncatedData;
        }
        const b0: u32 = @intCast(_.data[_.pos]);
        const b1: u32 = @intCast(_.data[_.pos + 1]);
        const b2: u32 = @intCast(_.data[_.pos + 2]);
        const b3: u32 = @intCast(_.data[_.pos + 3]);
        _.pos += 4;
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }

    pub fn readU64(*_): !u64 {
        if (_.remaining() < 8) {
            return DwarfError.TruncatedData;
        }
        const lo: u64 = @intCast(try _.readU32());
        const hi: u64 = @intCast(try _.readU32());
        return lo | (hi << 32);
    }

    pub fn readAddr(*_, addr_size: u8): !u64 {
        if (addr_size == 4) {
            const v = try _.readU32();
            return @intCast(v);
        }
        if (addr_size == 8) {
            return try _.readU64();
        }
        if (addr_size == 2) {
            const v = try _.readU16();
            return @intCast(v);
        }
        if (addr_size == 1) {
            const v = try _.readU8();
            return @intCast(v);
        }
        return DwarfError.InvalidHeader;
    }

    pub fn readBytes(*_, len: usize): ![]const u8 {
        if (_.remaining() < len) {
            return DwarfError.TruncatedData;
        }
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return slice;
    }

    pub fn readCString(*_): ![]const u8 {
        const start = _.pos;
        while (_.pos < _.data.len) {
            if (_.data[_.pos] == 0) {
                const s = _.data[start.._.pos];
                _.pos += 1;
                return s;
            }
            _.pos += 1;
        }
        return DwarfError.TruncatedData;
    }

    pub fn readUleb128(*_): !u64 {
        let result: u64 = 0;
        let shift: u6 = 0;
        let count: u32 = 0;
        while (count < 10) {
            const b = try _.readU8();
            const low: u64 = @intCast(b & 0x7f);
            result |= low << shift;
            if ((b & 0x80) == 0) {
                return result;
            }
            if (shift >= 57) {
                return DwarfError.InvalidLeb128;
            }
            shift += 7;
            count += 1;
        }
        return DwarfError.InvalidLeb128;
    }

    pub fn readSleb128(*_): !i64 {
        let result: i64 = 0;
        let shift: u6 = 0;
        let b: u8 = 0;
        let count: u32 = 0;
        while (count < 10) {
            b = try _.readU8();
            const low: i64 = @intCast(b & 0x7f);
            result |= low << shift;
            shift += 7;
            count += 1;
            if ((b & 0x80) == 0) {
                break;
            }
            if (shift >= 63) {
                return DwarfError.InvalidLeb128;
            }
        }
        if ((b & 0x80) != 0) {
            return DwarfError.InvalidLeb128;
        }
        if ((shift < 64) && ((b & 0x40) != 0)) {
            const neg_one: i64 = -1;
            result |= neg_one << shift;
        }
        return result;
    }
}

pub fn writeUleb128(alloc: std.mem.Allocator, out: *std.ArrayList(u8), value: u64): !void {
    let v = value;
    while (true) {
        let byte: u8 = @intCast(v & 0x7f);
        v >>= 7;
        if (v != 0) {
            byte |= 0x80;
        }
        try out.append(alloc, byte);
        if (v == 0) {
            break;
        }
    }
}

pub fn writeSleb128(alloc: std.mem.Allocator, out: *std.ArrayList(u8), value: i64): !void {
    let v = value;
    let more = true;
    while (more) {
        let byte: u8 = @intCast(v & 0x7f);
        v >>= 7;
        const sign_bit = (byte & 0x40) != 0;
        if ((v == 0 && !sign_bit) || (v == -1 && sign_bit)) {
            more = false;
        } else {
            byte |= 0x80;
        }
        try out.append(alloc, byte);
    }
}

pub fn writeU16(alloc: std.mem.Allocator, out: *std.ArrayList(u8), value: u16): !void {
    try out.append(alloc, @intCast(value & 0xff));
    try out.append(alloc, @intCast((value >> 8) & 0xff));
}

pub fn writeU32(alloc: std.mem.Allocator, out: *std.ArrayList(u8), value: u32): !void {
    try out.append(alloc, @intCast(value & 0xff));
    try out.append(alloc, @intCast((value >> 8) & 0xff));
    try out.append(alloc, @intCast((value >> 16) & 0xff));
    try out.append(alloc, @intCast((value >> 24) & 0xff));
}

pub fn writeU64(alloc: std.mem.Allocator, out: *std.ArrayList(u8), value: u64): !void {
    try writeU32(alloc, out, @intCast(value & 0xffffffff));
    try writeU32(alloc, out, @intCast((value >> 32) & 0xffffffff));
}
