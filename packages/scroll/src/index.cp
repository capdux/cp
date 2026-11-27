// @cap/scroll — Zero-allocation endian-aware binary reader/writer cursors
// Inspired by crates.io:scroll (https://github.com/m4b/scroll) & endian-type

import {
    readU16LE,
    readU16BE,
    readU32LE,
    readU32BE,
    readU64LE,
    readU64BE,
    writeU16LE,
    writeU16BE,
    writeU32LE,
    writeU32BE,
    writeU64LE,
    writeU64BE,
    findByteInBytes,
} from "cap:bin";

pub enum Endian {
    Little,
    Big,
}

pub struct PreadCursor {
    buf: []const u8,
    offset: usize,

    static fn new(buf: []const u8): PreadCursor {
        return PreadCursor{
            buf: buf,
            offset: 0,
        };
    }

    fn remaining(): usize {
        if (_.offset >= _.buf.len) { return 0; }
        return _.buf.len - _.offset;
    }

    fn readU8(*_): u8? {
        if (_.offset + 1 > _.buf.len) { return null; }
        const v = _.buf[_.offset];
        _.offset += 1;
        return v;
    }

    fn readU16(*_, endian: Endian): u16? {
        const v = if (endian == Endian.Little) readU16LE(_.buf, _.offset) else readU16BE(_.buf, _.offset);
        if (val := v) {
            _.offset += 2;
            return val;
        }
        return null;
    }

    fn readU32(*_, endian: Endian): u32? {
        const v = if (endian == Endian.Little) readU32LE(_.buf, _.offset) else readU32BE(_.buf, _.offset);
        if (val := v) {
            _.offset += 4;
            return val;
        }
        return null;
    }

    fn readU64(*_, endian: Endian): u64? {
        const v = if (endian == Endian.Little) readU64LE(_.buf, _.offset) else readU64BE(_.buf, _.offset);
        if (val := v) {
            _.offset += 8;
            return val;
        }
        return null;
    }

    fn readI32(*_, endian: Endian): i32? {
        if (u := _.readU32(endian)) {
            return @as(i32, @bitCast(u));
        }
        return null;
    }

    fn readCstr(*_): string? {
        if (i := findByteInBytes(_.buf, 0, _.offset)) {
            const start = _.offset;
            _.offset = i + 1;
            return _.buf[start..i];
        }
        return null;
    }
}

pub struct PwriteCursor {
    buf: []u8,
    offset: usize,

    static fn new(buf: []u8): PwriteCursor {
        return PwriteCursor{
            buf: buf,
            offset: 0,
        };
    }

    fn written(): usize {
        return _.offset;
    }

    fn remaining(): usize {
        if (_.offset >= _.buf.len) { return 0; }
        return _.buf.len - _.offset;
    }

    fn writeU8(*_, v: u8): bool {
        if (_.offset + 1 > _.buf.len) { return false; }
        _.buf[_.offset] = v;
        _.offset += 1;
        return true;
    }

    fn writeU16(*_, v: u16, endian: Endian): bool {
        const ok = if (endian == Endian.Little) writeU16LE(_.buf, _.offset, v) else writeU16BE(_.buf, _.offset, v);
        if (!ok) { return false; }
        _.offset += 2;
        return true;
    }

    fn writeU32(*_, v: u32, endian: Endian): bool {
        const ok = if (endian == Endian.Little) writeU32LE(_.buf, _.offset, v) else writeU32BE(_.buf, _.offset, v);
        if (!ok) { return false; }
        _.offset += 4;
        return true;
    }

    fn writeU64(*_, v: u64, endian: Endian): bool {
        const ok = if (endian == Endian.Little) writeU64LE(_.buf, _.offset, v) else writeU64BE(_.buf, _.offset, v);
        if (!ok) { return false; }
        _.offset += 8;
        return true;
    }
}
