// @cap/borsh — Zero-allocation canonical little-endian Borsh binary codec
// Inspired by NEAR / Solana `borsh`
// MIT License

import {
    readU16LE,
    readU32LE,
    readU64LE,
    writeU16LE,
    writeU32LE,
    writeU64LE,
} from "cap:bin";

const SIGN_BIT_64: u64 = (1 as u64) << 63;

fn i32ToBits(v: i32): u32 {
    if (v >= 0) { return v as u32; }
    let mag: u32 = (0 - (v + 1)) as u32;
    return ~mag;
}

fn bitsToI32(u: u32): i32 {
    if ((u & 0x80000000) == 0) { return u as i32; }
    let mag: u32 = ~u;
    return (0 - (mag as i32)) - 1;
}

fn i64ToBits(v: i64): u64 {
    if (v >= 0) { return v as u64; }
    let mag: u64 = (0 - (v + 1)) as u64;
    return ~mag;
}

fn bitsToI64(u: u64): i64 {
    if ((u & SIGN_BIT_64) == 0) { return u as i64; }
    let mag: u64 = ~u;
    return (0 - (mag as i64)) - 1;
}

pub struct BytesSpan {
    pub start: usize,
    pub len: usize,
    pub ok: bool,
}

pub struct BorshWriter {
    pub pos: usize,

    pub static fn new(): BorshWriter {
        return BorshWriter { pos: 0 };
    }

    pub fn writeU8(*_, out: []u8, v: u8): bool {
        if (_.pos >= out.len) { return false; }
        out[_.pos] = v;
        _.pos = _.pos + 1;
        return true;
    }

    pub fn writeBool(*_, out: []u8, v: bool): bool {
        return _.writeU8(out, if (v) 1 else 0);
    }

    pub fn writeOptionTag(*_, out: []u8, is_some: bool): bool {
        return _.writeBool(out, is_some);
    }

    pub fn writeU16Le(*_, out: []u8, v: u16): bool {
        if (!writeU16LE(out, _.pos, v)) { return false; }
        _.pos = _.pos + 2;
        return true;
    }

    pub fn writeU32Le(*_, out: []u8, v: u32): bool {
        if (!writeU32LE(out, _.pos, v)) { return false; }
        _.pos = _.pos + 4;
        return true;
    }

    pub fn writeU64Le(*_, out: []u8, v: u64): bool {
        if (!writeU64LE(out, _.pos, v)) { return false; }
        _.pos = _.pos + 8;
        return true;
    }

    pub fn writeI32Le(*_, out: []u8, v: i32): bool {
        return _.writeU32Le(out, i32ToBits(v));
    }

    pub fn writeI64Le(*_, out: []u8, v: i64): bool {
        return _.writeU64Le(out, i64ToBits(v));
    }

    pub fn writeBytes(*_, out: []u8, data: []const u8): bool {
        if (_.pos > out.len || out.len - _.pos < 4) { return false; }
        let avail: usize = (out.len - _.pos) - 4;
        if (data.len > avail) { return false; }
        if (!_.writeU32Le(out, data.len as u32)) { return false; }
        if (data.len > 0) {
            @memcpy(out[_.pos .. _.pos + data.len], data);
            _.pos = _.pos + data.len;
        }
        return true;
    }

    pub fn writeString(*_, out: []u8, s: string): bool {
        return _.writeBytes(out, s);
    }
}

pub struct BorshReader {
    pub pos: usize,
    pub ok: bool,

    pub static fn new(): BorshReader {
        return BorshReader { pos: 0, ok: true };
    }

    pub fn remaining(buf: []const u8): usize {
        return if (_.pos >= buf.len) 0 else buf.len - _.pos;
    }

    pub fn readU8(*_, buf: []const u8): u8 {
        if (!_.ok || _.pos >= buf.len) {
            _.ok = false;
            return 0;
        }
        let v: u8 = buf[_.pos];
        _.pos = _.pos + 1;
        return v;
    }

    pub fn readBool(*_, buf: []const u8): bool {
        let b: u8 = _.readU8(buf);
        if (!_.ok) { return false; }
        if (b == 0) { return false; }
        if (b == 1) { return true; }
        _.ok = false;
        return false;
    }

    pub fn readOptionTag(*_, buf: []const u8): bool {
        return _.readBool(buf);
    }

    pub fn readU16Le(*_, buf: []const u8): u16 {
        if (!_.ok) { return 0; }
        if (v := readU16LE(buf, _.pos)) {
            _.pos = _.pos + 2;
            return v;
        }
        _.ok = false;
        return 0;
    }

    pub fn readU32Le(*_, buf: []const u8): u32 {
        if (!_.ok) { return 0; }
        if (v := readU32LE(buf, _.pos)) {
            _.pos = _.pos + 4;
            return v;
        }
        _.ok = false;
        return 0;
    }

    pub fn readU64Le(*_, buf: []const u8): u64 {
        if (!_.ok) { return 0; }
        if (v := readU64LE(buf, _.pos)) {
            _.pos = _.pos + 8;
            return v;
        }
        _.ok = false;
        return 0;
    }

    pub fn readI32Le(*_, buf: []const u8): i32 {
        let bits: u32 = _.readU32Le(buf);
        return if (_.ok) bitsToI32(bits) else 0;
    }

    pub fn readI64Le(*_, buf: []const u8): i64 {
        let bits: u64 = _.readU64Le(buf);
        return if (_.ok) bitsToI64(bits) else 0;
    }

    pub fn readBytesSpan(*_, buf: []const u8): BytesSpan {
        let len_u32: u32 = _.readU32Le(buf);
        if (!_.ok) {
            return BytesSpan { start: 0, len: 0, ok: false };
        }
        let n: usize = len_u32 as usize;
        if (n > buf.len - _.pos) {
            _.ok = false;
            return BytesSpan { start: 0, len: 0, ok: false };
        }
        let start: usize = _.pos;
        _.pos = _.pos + n;
        return BytesSpan { start: start, len: n, ok: true };
    }
}
