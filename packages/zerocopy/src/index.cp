// @cap/zerocopy — Zero-copy byte parsing, endian wrappers, prefix/suffix & alignment-checked casts
// Modeled on crates.io `zerocopy` (FromBytes / IntoBytes / Ref / byteorder::{U16, U32, U64}).

import {
    readU16Le, readU16Be, readU32Le, readU32Be, readU64Le, readU64Be,
    readF32Le, readF64Le,
    writeU16Le, writeU16Be, writeU32Le, writeU32Be, writeU64Le, writeU64Be,
    writeF32Le, writeF64Le,
    castU16SliceAsBytes, castU32SliceAsBytes, castU64SliceAsBytes,
    tryCastSliceBytesToU32,
    checkCastLayout, POD_OK
} from "../../bytemuck/src/index.cp";

pub struct ByteCursor {
    pub pos: usize,
    pub total: usize,

    pub static fn new(total_len: usize): ByteCursor {
        return ByteCursor { pos: 0, total: total_len };
    }

    pub fn remaining(): usize {
        if (_.pos >= _.total) { return 0; }
        return _.total - _.pos;
    }

    pub fn reset(*_) {
        _.pos = 0;
    }

    pub fn seek(*_, offset: usize): bool {
        if (offset > _.total) { return false; }
        _.pos = offset;
        return true;
    }

    pub fn peekU8(buf: []const u8): u8? {
        if (_.pos + 1 > buf.len || _.pos + 1 > _.total) { return null; }
        return buf[_.pos];
    }

    pub fn nextU8(*_, buf: []u8): u8? {
        if (_.pos + 1 > buf.len || _.pos + 1 > _.total) { return null; }
        let v = buf[_.pos];
        _.pos = _.pos + 1;
        return v;
    }

    pub fn nextU16Le(*_, buf: []u8): u16? {
        if (_.pos + 2 > buf.len || _.pos + 2 > _.total) { return null; }
        let v = readU16Le(buf[_.pos..(_.pos + 2)]);
        if (v != null) { _.pos = _.pos + 2; }
        return v;
    }

    pub fn nextU16Be(*_, buf: []u8): u16? {
        if (_.pos + 2 > buf.len || _.pos + 2 > _.total) { return null; }
        let v = readU16Be(buf[_.pos..(_.pos + 2)]);
        if (v != null) { _.pos = _.pos + 2; }
        return v;
    }

    pub fn nextU32Le(*_, buf: []u8): u32? {
        if (_.pos + 4 > buf.len || _.pos + 4 > _.total) { return null; }
        let v = readU32Le(buf[_.pos..(_.pos + 4)]);
        if (v != null) { _.pos = _.pos + 4; }
        return v;
    }

    pub fn nextU32Be(*_, buf: []u8): u32? {
        if (_.pos + 4 > buf.len || _.pos + 4 > _.total) { return null; }
        let v = readU32Be(buf[_.pos..(_.pos + 4)]);
        if (v != null) { _.pos = _.pos + 4; }
        return v;
    }

    pub fn nextU64Le(*_, buf: []u8): u64? {
        if (_.pos + 8 > buf.len || _.pos + 8 > _.total) { return null; }
        let v = readU64Le(buf[_.pos..(_.pos + 8)]);
        if (v != null) { _.pos = _.pos + 8; }
        return v;
    }

    pub fn nextU64Be(*_, buf: []u8): u64? {
        if (_.pos + 8 > buf.len || _.pos + 8 > _.total) { return null; }
        let v = readU64Be(buf[_.pos..(_.pos + 8)]);
        if (v != null) { _.pos = _.pos + 8; }
        return v;
    }

    pub fn nextI32Le(*_, buf: []u8): i32? {
        const raw = _.nextU32Le(buf);
        if (raw == null) { return null; }
        const res: i32 = @bitCast(raw.?);
        return res;
    }

    pub fn nextF32Le(*_, buf: []const u8): f32? {
        if (_.pos + 4 > buf.len || _.pos + 4 > _.total) { return null; }
        let v = readF32Le(buf[_.pos..(_.pos + 4)]);
        if (v != null) { _.pos = _.pos + 4; }
        return v;
    }

    pub fn nextF64Le(*_, buf: []const u8): f64? {
        if (_.pos + 8 > buf.len || _.pos + 8 > _.total) { return null; }
        let v = readF64Le(buf[_.pos..(_.pos + 8)]);
        if (v != null) { _.pos = _.pos + 8; }
        return v;
    }

    pub fn nextBytes(*_, buf: []const u8, n: usize): ([]const u8)? {
        if (_.pos + n > buf.len || _.pos + n > _.total) { return null; }
        const slice = buf[_.pos..(_.pos + n)];
        _.pos = _.pos + n;
        return slice;
    }

    pub fn skip(*_, n: usize): bool {
        if (_.pos + n > _.total) { return false; }
        _.pos = _.pos + n;
        return true;
    }
}

pub struct ByteWriterCursor {
    pub pos: usize,
    pub cap: usize,

    pub static fn new(cap_len: usize): ByteWriterCursor {
        return ByteWriterCursor { pos: 0, cap: cap_len };
    }

    pub fn written(): usize {
        return _.pos;
    }

    pub fn remaining(): usize {
        if (_.pos >= _.cap) { return 0; }
        return _.cap - _.pos;
    }

    pub fn reset(*_) {
        _.pos = 0;
    }

    pub fn putU8(*_, out: []u8, v: u8): bool {
        if (_.pos + 1 > out.len || _.pos + 1 > _.cap) { return false; }
        out[_.pos] = v;
        _.pos = _.pos + 1;
        return true;
    }

    pub fn putU16Le(*_, out: []u8, v: u16): bool {
        if (_.pos + 2 > out.len || _.pos + 2 > _.cap) { return false; }
        if (!writeU16Le(v, out[_.pos..(_.pos + 2)])) { return false; }
        _.pos = _.pos + 2;
        return true;
    }

    pub fn putU16Be(*_, out: []u8, v: u16): bool {
        if (_.pos + 2 > out.len || _.pos + 2 > _.cap) { return false; }
        if (!writeU16Be(v, out[_.pos..(_.pos + 2)])) { return false; }
        _.pos = _.pos + 2;
        return true;
    }

    pub fn putU32Le(*_, out: []u8, v: u32): bool {
        if (_.pos + 4 > out.len || _.pos + 4 > _.cap) { return false; }
        if (!writeU32Le(v, out[_.pos..(_.pos + 4)])) { return false; }
        _.pos = _.pos + 4;
        return true;
    }

    pub fn putU32Be(*_, out: []u8, v: u32): bool {
        if (_.pos + 4 > out.len || _.pos + 4 > _.cap) { return false; }
        if (!writeU32Be(v, out[_.pos..(_.pos + 4)])) { return false; }
        _.pos = _.pos + 4;
        return true;
    }

    pub fn putU64Le(*_, out: []u8, v: u64): bool {
        if (_.pos + 8 > out.len || _.pos + 8 > _.cap) { return false; }
        if (!writeU64Le(v, out[_.pos..(_.pos + 8)])) { return false; }
        _.pos = _.pos + 8;
        return true;
    }

    pub fn putU64Be(*_, out: []u8, v: u64): bool {
        if (_.pos + 8 > out.len || _.pos + 8 > _.cap) { return false; }
        if (!writeU64Be(v, out[_.pos..(_.pos + 8)])) { return false; }
        _.pos = _.pos + 8;
        return true;
    }

    pub fn putI32Le(*_, out: []u8, v: i32): bool {
        const bits: u32 = @bitCast(v);
        return _.putU32Le(out, bits);
    }

    pub fn putF32Le(*_, out: []u8, v: f32): bool {
        if (_.pos + 4 > out.len || _.pos + 4 > _.cap) { return false; }
        if (!writeF32Le(v, out[_.pos..(_.pos + 4)])) { return false; }
        _.pos = _.pos + 4;
        return true;
    }

    pub fn putF64Le(*_, out: []u8, v: f64): bool {
        if (_.pos + 8 > out.len || _.pos + 8 > _.cap) { return false; }
        if (!writeF64Le(v, out[_.pos..(_.pos + 8)])) { return false; }
        _.pos = _.pos + 8;
        return true;
    }

    pub fn putBytes(*_, out: []u8, src: []const u8): bool {
        if (_.pos + src.len > out.len || _.pos + src.len > _.cap) { return false; }
        let i: usize = 0;
        while (i < src.len) : (i += 1) {
            out[_.pos + i] = src[i];
        }
        _.pos = _.pos + src.len;
        return true;
    }
}

pub fn isAlignedSlice(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): bool {
    return checkCastLayout(byte_len, elem_size, addr, align_req) == POD_OK;
}

// --- zerocopy::byteorder wrappers (unaligned byte-backed endian integers) ---

pub struct U16LE {
    pub bytes: [2]u8,

    pub static fn new(v: u16): U16LE {
        return U16LE { bytes: [(v & 0xFF) as u8, ((v >> 8) & 0xFF) as u8] };
    }

    pub static fn fromBytes(buf: []const u8): U16LE? {
        if (buf.len < 2) { return null; }
        return U16LE { bytes: [buf[0], buf[1]] };
    }

    pub fn get(): u16 {
        return (_.bytes[0] as u16) | ((_.bytes[1] as u16) << 8);
    }

    pub fn set(*_, v: u16) {
        _.bytes[0] = (v & 0xFF) as u8;
        _.bytes[1] = ((v >> 8) & 0xFF) as u8;
    }

    pub fn toBytes(): [2]u8 { return _.bytes; }
}

pub struct U16BE {
    pub bytes: [2]u8,

    pub static fn new(v: u16): U16BE {
        return U16BE { bytes: [((v >> 8) & 0xFF) as u8, (v & 0xFF) as u8] };
    }

    pub static fn fromBytes(buf: []const u8): U16BE? {
        if (buf.len < 2) { return null; }
        return U16BE { bytes: [buf[0], buf[1]] };
    }

    pub fn get(): u16 {
        return ((_.bytes[0] as u16) << 8) | (_.bytes[1] as u16);
    }

    pub fn set(*_, v: u16) {
        _.bytes[0] = ((v >> 8) & 0xFF) as u8;
        _.bytes[1] = (v & 0xFF) as u8;
    }

    pub fn toBytes(): [2]u8 { return _.bytes; }
}

pub struct U32LE {
    pub bytes: [4]u8,

    pub static fn new(v: u32): U32LE {
        return U32LE {
            bytes: [
                (v & 0xFF) as u8,
                ((v >> 8) & 0xFF) as u8,
                ((v >> 16) & 0xFF) as u8,
                ((v >> 24) & 0xFF) as u8,
            ],
        };
    }

    pub static fn fromBytes(buf: []const u8): U32LE? {
        if (buf.len < 4) { return null; }
        return U32LE { bytes: [buf[0], buf[1], buf[2], buf[3]] };
    }

    pub fn get(): u32 {
        return (_.bytes[0] as u32)
            | ((_.bytes[1] as u32) << 8)
            | ((_.bytes[2] as u32) << 16)
            | ((_.bytes[3] as u32) << 24);
    }

    pub fn set(*_, v: u32) {
        _.bytes[0] = (v & 0xFF) as u8;
        _.bytes[1] = ((v >> 8) & 0xFF) as u8;
        _.bytes[2] = ((v >> 16) & 0xFF) as u8;
        _.bytes[3] = ((v >> 24) & 0xFF) as u8;
    }

    pub fn toBytes(): [4]u8 { return _.bytes; }
}

pub struct U32BE {
    pub bytes: [4]u8,

    pub static fn new(v: u32): U32BE {
        return U32BE {
            bytes: [
                ((v >> 24) & 0xFF) as u8,
                ((v >> 16) & 0xFF) as u8,
                ((v >> 8) & 0xFF) as u8,
                (v & 0xFF) as u8,
            ],
        };
    }

    pub static fn fromBytes(buf: []const u8): U32BE? {
        if (buf.len < 4) { return null; }
        return U32BE { bytes: [buf[0], buf[1], buf[2], buf[3]] };
    }

    pub fn get(): u32 {
        return ((_.bytes[0] as u32) << 24)
            | ((_.bytes[1] as u32) << 16)
            | ((_.bytes[2] as u32) << 8)
            | (_.bytes[3] as u32);
    }

    pub fn set(*_, v: u32) {
        _.bytes[0] = ((v >> 24) & 0xFF) as u8;
        _.bytes[1] = ((v >> 16) & 0xFF) as u8;
        _.bytes[2] = ((v >> 8) & 0xFF) as u8;
        _.bytes[3] = (v & 0xFF) as u8;
    }

    pub fn toBytes(): [4]u8 { return _.bytes; }
}

pub struct U64LE {
    pub bytes: [8]u8,

    pub static fn new(v: u64): U64LE {
        let out: [8]u8 = [0; 8];
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            out[i] = ((v >> ((i * 8) as u6)) & 0xFF) as u8;
        }
        return U64LE { bytes: out };
    }

    pub static fn fromBytes(buf: []const u8): U64LE? {
        if (buf.len < 8) { return null; }
        return U64LE {
            bytes: [buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]],
        };
    }

    pub fn get(): u64 {
        let v: u64 = 0;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            v = v | ((_.bytes[i] as u64) << ((i * 8) as u6));
        }
        return v;
    }

    pub fn set(*_, v: u64) {
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            _.bytes[i] = ((v >> ((i * 8) as u6)) & 0xFF) as u8;
        }
    }

    pub fn toBytes(): [8]u8 { return _.bytes; }
}

pub struct U64BE {
    pub bytes: [8]u8,

    pub static fn new(v: u64): U64BE {
        let out: [8]u8 = [0; 8];
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            out[7 - i] = ((v >> ((i * 8) as u6)) & 0xFF) as u8;
        }
        return U64BE { bytes: out };
    }

    pub static fn fromBytes(buf: []const u8): U64BE? {
        if (buf.len < 8) { return null; }
        return U64BE {
            bytes: [buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]],
        };
    }

    pub fn get(): u64 {
        let v: u64 = 0;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            v = (v << 8) | (_.bytes[i] as u64);
        }
        return v;
    }

    pub fn set(*_, v: u64) {
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            _.bytes[7 - i] = ((v >> ((i * 8) as u6)) & 0xFF) as u8;
        }
    }

    pub fn toBytes(): [8]u8 { return _.bytes; }
}

// --- FromBytes::read_from_prefix / read_from_suffix / IntoBytes::write_to_prefix ---

pub struct SplitBytes {
    pub head: []const u8,
    pub tail: []const u8,
}

pub struct PrefixU16 {
    pub value: u16,
    pub rest: []const u8,
}

pub struct PrefixU32 {
    pub value: u32,
    pub rest: []const u8,
}

pub struct PrefixU64 {
    pub value: u64,
    pub rest: []const u8,
}

/// Splits `n` bytes from the front of `buf` (`zerocopy::FromBytes::read_from_prefix`).
pub fn readFromPrefix(buf: []const u8, n: usize): SplitBytes? {
    if (buf.len < n) { return null; }
    return SplitBytes { head: buf[0..n], tail: buf[n..buf.len] };
}

/// Splits `n` bytes from the end of `buf` (`zerocopy::FromBytes::read_from_suffix`).
pub fn readFromSuffix(buf: []const u8, n: usize): SplitBytes? {
    if (buf.len < n) { return null; }
    const split = buf.len - n;
    return SplitBytes { head: buf[0..split], tail: buf[split..buf.len] };
}

pub fn readU16LeFromPrefix(buf: []const u8): PrefixU16? {
    const v = U16LE.fromBytes(buf);
    if (v == null) { return null; }
    return PrefixU16 { value: v.?.get(), rest: buf[2..buf.len] };
}

pub fn readU16BeFromPrefix(buf: []const u8): PrefixU16? {
    const v = U16BE.fromBytes(buf);
    if (v == null) { return null; }
    return PrefixU16 { value: v.?.get(), rest: buf[2..buf.len] };
}

pub fn readU32LeFromPrefix(buf: []const u8): PrefixU32? {
    const v = U32LE.fromBytes(buf);
    if (v == null) { return null; }
    return PrefixU32 { value: v.?.get(), rest: buf[4..buf.len] };
}

pub fn readU32BeFromPrefix(buf: []const u8): PrefixU32? {
    const v = U32BE.fromBytes(buf);
    if (v == null) { return null; }
    return PrefixU32 { value: v.?.get(), rest: buf[4..buf.len] };
}

pub fn readU64LeFromPrefix(buf: []const u8): PrefixU64? {
    const v = U64LE.fromBytes(buf);
    if (v == null) { return null; }
    return PrefixU64 { value: v.?.get(), rest: buf[8..buf.len] };
}

pub fn readU64BeFromPrefix(buf: []const u8): PrefixU64? {
    const v = U64BE.fromBytes(buf);
    if (v == null) { return null; }
    return PrefixU64 { value: v.?.get(), rest: buf[8..buf.len] };
}

pub fn readU16LeFromSuffix(buf: []const u8): PrefixU16? {
    if (buf.len < 2) { return null; }
    const split = buf.len - 2;
    const v = U16LE.fromBytes(buf[split..buf.len]);
    if (v == null) { return null; }
    return PrefixU16 { value: v.?.get(), rest: buf[0..split] };
}

pub fn readU32LeFromSuffix(buf: []const u8): PrefixU32? {
    if (buf.len < 4) { return null; }
    const split = buf.len - 4;
    const v = U32LE.fromBytes(buf[split..buf.len]);
    if (v == null) { return null; }
    return PrefixU32 { value: v.?.get(), rest: buf[0..split] };
}

pub fn readU64LeFromSuffix(buf: []const u8): PrefixU64? {
    if (buf.len < 8) { return null; }
    const split = buf.len - 8;
    const v = U64LE.fromBytes(buf[split..buf.len]);
    if (v == null) { return null; }
    return PrefixU64 { value: v.?.get(), rest: buf[0..split] };
}

/// Copies `src` into the prefix of `dst` and returns the remaining tail slice (`write_to_prefix`).
pub fn writeToPrefix(dst: []u8, src: []const u8): ([]u8)? {
    if (dst.len < src.len) { return null; }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
    return dst[src.len..dst.len];
}

/// Copies `src` into the suffix of `dst` and returns the leading prefix slice (`write_to_suffix`).
pub fn writeToSuffix(dst: []u8, src: []const u8): ([]u8)? {
    if (dst.len < src.len) { return null; }
    const start = dst.len - src.len;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[start + i] = src[i];
    }
    return dst[0..start];
}

// --- Alignment-checked refFromBytes & zero-copy slice conversions ---

/// Alignment-checked & exact-size `ref_from_bytes` for `u16` (2-byte aligned, len == 2).
pub fn refFromBytesU16(buf: []const u8): u16? {
    unsafe {
        const addr = @intFromPtr(buf.ptr);
        if (buf.len != 2 || checkCastLayout(buf.len, 2, addr, 2) != POD_OK) {
            return null;
        }
        const ptr: *const u16 = @ptrCast(@alignCast(buf.ptr));
        return ptr.*;
    }
}

/// Alignment-checked & exact-size `ref_from_bytes` for `u32` (4-byte aligned, len == 4).
pub fn refFromBytesU32(buf: []const u8): u32? {
    unsafe {
        const addr = @intFromPtr(buf.ptr);
        if (buf.len != 4 || checkCastLayout(buf.len, 4, addr, 4) != POD_OK) {
            return null;
        }
        const ptr: *const u32 = @ptrCast(@alignCast(buf.ptr));
        return ptr.*;
    }
}

/// Alignment-checked & exact-size `ref_from_bytes` for `u64` (8-byte aligned, len == 8).
pub fn refFromBytesU64(buf: []const u8): u64? {
    unsafe {
        const addr = @intFromPtr(buf.ptr);
        if (buf.len != 8 || checkCastLayout(buf.len, 8, addr, 8) != POD_OK) {
            return null;
        }
        const ptr: *const u64 = @ptrCast(@alignCast(buf.ptr));
        return ptr.*;
    }
}

/// Alignment-checked zero-copy view of `[]const u8` as `[]const u32` (`Ref::from_bytes`).
pub fn refFromBytesSliceU32(buf: []const u8): ([]const u32)? {
    return tryCastSliceBytesToU32(buf);
}

pub fn sliceAsBytesU16(src: []const u16): []const u8 {
    return castU16SliceAsBytes(src);
}

pub fn sliceAsBytesU32(src: []const u32): []const u8 {
    return castU32SliceAsBytes(src);
}

pub fn sliceAsBytesU64(src: []const u64): []const u8 {
    return castU64SliceAsBytes(src);
}
