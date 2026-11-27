// @cap/rend — Zero-allocation endian-aware primitive types
// Inspired by crates.io:rend (https://github.com/rkyv/rend)

import {
    readU16BE,
    readU16LE,
    readU32BE,
    readU32LE,
    readU64BE,
    readU64LE,
} from "cap:bin";

fn rndPackU16Be(v: u16): [2]u8 {
    let out: [2]u8 = undefined;
    out[0] = u8((v >> 8) & 0xFF);
    out[1] = u8(v & 0xFF);
    return out;
}

fn rndPackU16Le(v: u16): [2]u8 {
    let out: [2]u8 = undefined;
    out[0] = u8(v & 0xFF);
    out[1] = u8((v >> 8) & 0xFF);
    return out;
}

fn rndUnpackU16Be(b: [2]u8): u16 {
    return (u16(b[0]) << 8) | u16(b[1]);
}

fn rndUnpackU16Le(b: [2]u8): u16 {
    return u16(b[0]) | (u16(b[1]) << 8);
}

fn rndPackU32Be(v: u32): [4]u8 {
    let out: [4]u8 = undefined;
    out[0] = u8((v >> 24) & 0xFF);
    out[1] = u8((v >> 16) & 0xFF);
    out[2] = u8((v >> 8) & 0xFF);
    out[3] = u8(v & 0xFF);
    return out;
}

fn rndPackU32Le(v: u32): [4]u8 {
    let out: [4]u8 = undefined;
    out[0] = u8(v & 0xFF);
    out[1] = u8((v >> 8) & 0xFF);
    out[2] = u8((v >> 16) & 0xFF);
    out[3] = u8((v >> 24) & 0xFF);
    return out;
}

fn rndUnpackU32Be(b: [4]u8): u32 {
    return (u32(b[0]) << 24) | (u32(b[1]) << 16) | (u32(b[2]) << 8) | u32(b[3]);
}

fn rndUnpackU32Le(b: [4]u8): u32 {
    return u32(b[0]) | (u32(b[1]) << 8) | (u32(b[2]) << 16) | (u32(b[3]) << 24);
}

fn rndPackU64Be(v: u64): [8]u8 {
    let out: [8]u8 = undefined;
    out[0] = u8((v >> 56) & 0xFF);
    out[1] = u8((v >> 48) & 0xFF);
    out[2] = u8((v >> 40) & 0xFF);
    out[3] = u8((v >> 32) & 0xFF);
    out[4] = u8((v >> 24) & 0xFF);
    out[5] = u8((v >> 16) & 0xFF);
    out[6] = u8((v >> 8) & 0xFF);
    out[7] = u8(v & 0xFF);
    return out;
}

fn rndPackU64Le(v: u64): [8]u8 {
    let out: [8]u8 = undefined;
    out[0] = u8(v & 0xFF);
    out[1] = u8((v >> 8) & 0xFF);
    out[2] = u8((v >> 16) & 0xFF);
    out[3] = u8((v >> 24) & 0xFF);
    out[4] = u8((v >> 32) & 0xFF);
    out[5] = u8((v >> 40) & 0xFF);
    out[6] = u8((v >> 48) & 0xFF);
    out[7] = u8((v >> 56) & 0xFF);
    return out;
}

fn rndUnpackU64Be(b: [8]u8): u64 {
    return (u64(b[0]) << 56)
        | (u64(b[1]) << 48)
        | (u64(b[2]) << 40)
        | (u64(b[3]) << 32)
        | (u64(b[4]) << 24)
        | (u64(b[5]) << 16)
        | (u64(b[6]) << 8)
        | u64(b[7]);
}

fn rndUnpackU64Le(b: [8]u8): u64 {
    return u64(b[0])
        | (u64(b[1]) << 8)
        | (u64(b[2]) << 16)
        | (u64(b[3]) << 24)
        | (u64(b[4]) << 32)
        | (u64(b[5]) << 40)
        | (u64(b[6]) << 48)
        | (u64(b[7]) << 56);
}

pub struct U16Be {
    bytes: [2]u8,

    static fn fromNative(v: u16): U16Be {
        return U16Be{ bytes: rndPackU16Be(v) };
    }

    static fn fromBytes(b: [2]u8): U16Be {
        return U16Be{ bytes: b };
    }

    fn toNative(): u16 {
        return rndUnpackU16Be(_.bytes);
    }

    fn asBytes(): [2]u8 {
        const out: [2]u8 = _.bytes;
        return out;
    }
}

pub struct U16Le {
    bytes: [2]u8,

    static fn fromNative(v: u16): U16Le {
        return U16Le{ bytes: rndPackU16Le(v) };
    }

    static fn fromBytes(b: [2]u8): U16Le {
        return U16Le{ bytes: b };
    }

    fn toNative(): u16 {
        return rndUnpackU16Le(_.bytes);
    }

    fn asBytes(): [2]u8 {
        const out: [2]u8 = _.bytes;
        return out;
    }
}

pub struct U32Be {
    bytes: [4]u8,

    static fn fromNative(v: u32): U32Be {
        return U32Be{ bytes: rndPackU32Be(v) };
    }

    static fn fromBytes(b: [4]u8): U32Be {
        return U32Be{ bytes: b };
    }

    fn toNative(): u32 {
        return rndUnpackU32Be(_.bytes);
    }

    fn asBytes(): [4]u8 {
        const out: [4]u8 = _.bytes;
        return out;
    }
}

pub struct U32Le {
    bytes: [4]u8,

    static fn fromNative(v: u32): U32Le {
        return U32Le{ bytes: rndPackU32Le(v) };
    }

    static fn fromBytes(b: [4]u8): U32Le {
        return U32Le{ bytes: b };
    }

    fn toNative(): u32 {
        return rndUnpackU32Le(_.bytes);
    }

    fn asBytes(): [4]u8 {
        const out: [4]u8 = _.bytes;
        return out;
    }
}

pub struct I32Be {
    bytes: [4]u8,

    static fn fromNative(v: i32): I32Be {
        const u = @as(u32, @bitCast(v));
        return I32Be{ bytes: rndPackU32Be(u) };
    }

    static fn fromBytes(b: [4]u8): I32Be {
        return I32Be{ bytes: b };
    }

    fn toNative(): i32 {
        const u: u32 = rndUnpackU32Be(_.bytes);
        return @as(i32, @bitCast(u));
    }

    fn asBytes(): [4]u8 {
        const out: [4]u8 = _.bytes;
        return out;
    }
}

pub struct I32Le {
    bytes: [4]u8,

    static fn fromNative(v: i32): I32Le {
        const u = @as(u32, @bitCast(v));
        return I32Le{ bytes: rndPackU32Le(u) };
    }

    static fn fromBytes(b: [4]u8): I32Le {
        return I32Le{ bytes: b };
    }

    fn toNative(): i32 {
        const u: u32 = rndUnpackU32Le(_.bytes);
        return @as(i32, @bitCast(u));
    }

    fn asBytes(): [4]u8 {
        const out: [4]u8 = _.bytes;
        return out;
    }
}

pub struct U64Be {
    bytes: [8]u8,

    static fn fromNative(v: u64): U64Be {
        return U64Be{ bytes: rndPackU64Be(v) };
    }

    static fn fromBytes(b: [8]u8): U64Be {
        return U64Be{ bytes: b };
    }

    fn toNative(): u64 {
        return rndUnpackU64Be(_.bytes);
    }

    fn asBytes(): [8]u8 {
        const out: [8]u8 = _.bytes;
        return out;
    }
}

pub struct U64Le {
    bytes: [8]u8,

    static fn fromNative(v: u64): U64Le {
        return U64Le{ bytes: rndPackU64Le(v) };
    }

    static fn fromBytes(b: [8]u8): U64Le {
        return U64Le{ bytes: b };
    }

    fn toNative(): u64 {
        return rndUnpackU64Le(_.bytes);
    }

    fn asBytes(): [8]u8 {
        const out: [8]u8 = _.bytes;
        return out;
    }
}

pub struct I64Be {
    bytes: [8]u8,

    static fn fromNative(v: i64): I64Be {
        const u = @as(u64, @bitCast(v));
        return I64Be{ bytes: rndPackU64Be(u) };
    }

    static fn fromBytes(b: [8]u8): I64Be {
        return I64Be{ bytes: b };
    }

    fn toNative(): i64 {
        const u: u64 = rndUnpackU64Be(_.bytes);
        return @as(i64, @bitCast(u));
    }

    fn asBytes(): [8]u8 {
        const out: [8]u8 = _.bytes;
        return out;
    }
}

pub struct I64Le {
    bytes: [8]u8,

    static fn fromNative(v: i64): I64Le {
        const u = @as(u64, @bitCast(v));
        return I64Le{ bytes: rndPackU64Le(u) };
    }

    static fn fromBytes(b: [8]u8): I64Le {
        return I64Le{ bytes: b };
    }

    fn toNative(): i64 {
        const u: u64 = rndUnpackU64Le(_.bytes);
        return @as(i64, @bitCast(u));
    }

    fn asBytes(): [8]u8 {
        const out: [8]u8 = _.bytes;
        return out;
    }
}

pub fn readU16Be(buf: []const u8, offset: usize): u16? {
    if (offset > buf.len || buf.len - offset < 2) {
        return null;
    }
    return readU16BE(buf, offset);
}

pub fn readU16Le(buf: []const u8, offset: usize): u16? {
    if (offset > buf.len || buf.len - offset < 2) {
        return null;
    }
    return readU16LE(buf, offset);
}

pub fn readU32Be(buf: []const u8, offset: usize): u32? {
    if (offset > buf.len || buf.len - offset < 4) {
        return null;
    }
    return readU32BE(buf, offset);
}

pub fn readU32Le(buf: []const u8, offset: usize): u32? {
    if (offset > buf.len || buf.len - offset < 4) {
        return null;
    }
    return readU32LE(buf, offset);
}

pub fn readU64Be(buf: []const u8, offset: usize): u64? {
    if (offset > buf.len || buf.len - offset < 8) {
        return null;
    }
    return readU64BE(buf, offset);
}

pub fn readU64Le(buf: []const u8, offset: usize): u64? {
    if (offset > buf.len || buf.len - offset < 8) {
        return null;
    }
    return readU64LE(buf, offset);
}
