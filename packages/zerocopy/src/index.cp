import {
    readU16Le, readU32Le, readU32Be, readU64Le,
    writeU16Le, writeU32Le, writeU32Be, writeU64Le,
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
}

pub fn isAlignedSlice(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): bool {
    return checkCastLayout(byte_len, elem_size, addr, align_req) == POD_OK;
}
