// @cap/byteorder — Zero-heap Endian Codecs & Bounds-Checked Untrusted Binary Reader/Writer
// Clean-room MIT implementation in pure CP (inspired by Rust `byteorder` [993M] + `untrusted` [1.02B]).

import {
    readU16BE,
    readU16LE,
    readU32BE,
    readU32LE,
    readU64BE,
    readU64LE,
    writeU16BE,
    writeU16LE,
    writeU32BE,
    writeU32LE,
    writeU64BE,
    writeU64LE,
} from "cap:bin";
import { writeStr } from "cap:fmt";

// ============================================================================
// Stateless Big-Endian / Little-Endian Slice Codecs
// ============================================================================

pub fn readU16Be(buf: string, off: usize) -> u16? { return readU16BE(buf, off); }
pub fn readU16Le(buf: string, off: usize) -> u16? { return readU16LE(buf, off); }
pub fn readU32Be(buf: string, off: usize) -> u32? { return readU32BE(buf, off); }
pub fn readU32Le(buf: string, off: usize) -> u32? { return readU32LE(buf, off); }
pub fn readU64Be(buf: string, off: usize) -> u64? { return readU64BE(buf, off); }
pub fn readU64Le(buf: string, off: usize) -> u64? { return readU64LE(buf, off); }

pub fn writeU16Be(out: []u8, off: usize, v: u16) -> bool { return writeU16BE(out, off, v); }
pub fn writeU16Le(out: []u8, off: usize, v: u16) -> bool { return writeU16LE(out, off, v); }
pub fn writeU32Be(out: []u8, off: usize, v: u32) -> bool { return writeU32BE(out, off, v); }
pub fn writeU32Le(out: []u8, off: usize, v: u32) -> bool { return writeU32LE(out, off, v); }
pub fn writeU64Be(out: []u8, off: usize, v: u64) -> bool { return writeU64BE(out, off, v); }
pub fn writeU64Le(out: []u8, off: usize, v: u64) -> bool { return writeU64LE(out, off, v); }

// ============================================================================
// Bounds-Checked Forward-Only Untrusted ByteReader (Rust `untrusted` model)
// ============================================================================

pub struct ByteReader {
    pub data: string,
    pub pos: usize,

    pub static fn init(data: string) -> ByteReader {
        return ByteReader { data: data, pos: 0 };
    }

    pub fn remaining() -> usize {
        return _.data.len - _.pos;
    }

    pub fn atEnd() -> bool {
        return _.pos >= _.data.len;
    }

    pub fn peekU8() -> u8? {
        if (_.pos >= _.data.len) { return null; }
        return _.data[_.pos];
    }

    pub fn readU8(*_) -> u8? {
        if (_.pos >= _.data.len) { return null; }
        const b: u8 = _.data[_.pos];
        _.pos += 1;
        return b;
    }

    pub fn readU16Be(*_) -> u16? {
        if (v := readU16BE(_.data, _.pos)) { _.pos += 2; return v; }
        return null;
    }

    pub fn readU16Le(*_) -> u16? {
        if (v := readU16LE(_.data, _.pos)) { _.pos += 2; return v; }
        return null;
    }

    pub fn readU32Be(*_) -> u32? {
        if (v := readU32BE(_.data, _.pos)) { _.pos += 4; return v; }
        return null;
    }

    pub fn readU32Le(*_) -> u32? {
        if (v := readU32LE(_.data, _.pos)) { _.pos += 4; return v; }
        return null;
    }

    pub fn readU64Be(*_) -> u64? {
        if (v := readU64BE(_.data, _.pos)) { _.pos += 8; return v; }
        return null;
    }

    pub fn readU64Le(*_) -> u64? {
        if (v := readU64LE(_.data, _.pos)) { _.pos += 8; return v; }
        return null;
    }

    pub fn readBytes(*_, n: usize) -> string? {
        if (n > _.data.len - _.pos) { return null; }
        const start: usize = _.pos;
        const end: usize = start + n;
        _.pos = end;
        const buf: string = _.data;
        return buf[start..end];
    }

    pub fn skip(*_, n: usize) -> bool {
        if (n > _.data.len - _.pos) { return false; }
        _.pos += n;
        return true;
    }

    pub fn readToEnd(*_) -> string {
        const start: usize = _.pos;
        _.pos = _.data.len;
        const buf: string = _.data;
        return buf[start..buf.len];
    }
}

// ============================================================================
// Bounds-Checked Zero-Heap ByteWriter
// ============================================================================

pub struct ByteWriter {
    pub out: []u8,
    pub pos: usize,
    pub overflow: bool,

    pub static fn init(out: []u8) -> ByteWriter {
        return ByteWriter { out: out, pos: 0, overflow: false };
    }

    pub fn asSlice() -> string {
        return _.out[0.._.pos];
    }

    pub fn writeU8(*_, v: u8) -> bool {
        if (_.pos >= _.out.len) { _.overflow = true; return false; }
        _.out[_.pos] = v;
        _.pos += 1;
        return true;
    }

    pub fn writeU16Be(*_, v: u16) -> bool {
        if (!writeU16BE(_.out, _.pos, v)) { _.overflow = true; return false; }
        _.pos += 2;
        return true;
    }

    pub fn writeU16Le(*_, v: u16) -> bool {
        if (!writeU16LE(_.out, _.pos, v)) { _.overflow = true; return false; }
        _.pos += 2;
        return true;
    }

    pub fn writeU32Be(*_, v: u32) -> bool {
        if (!writeU32BE(_.out, _.pos, v)) { _.overflow = true; return false; }
        _.pos += 4;
        return true;
    }

    pub fn writeU32Le(*_, v: u32) -> bool {
        if (!writeU32LE(_.out, _.pos, v)) { _.overflow = true; return false; }
        _.pos += 4;
        return true;
    }

    pub fn writeU64Be(*_, v: u64) -> bool {
        if (!writeU64BE(_.out, _.pos, v)) { _.overflow = true; return false; }
        _.pos += 8;
        return true;
    }

    pub fn writeU64Le(*_, v: u64) -> bool {
        if (!writeU64LE(_.out, _.pos, v)) { _.overflow = true; return false; }
        _.pos += 8;
        return true;
    }

    pub fn writeBytes(*_, s: string) -> bool {
        if (s.len > _.out.len - _.pos) { _.overflow = true; return false; }
        _.pos = writeStr(s, _.out, _.pos);
        return true;
    }
}
