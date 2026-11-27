// @cap/scroll — Zero-allocation endian-aware binary reader/writer cursors
// Inspired by crates.io:scroll (https://github.com/m4b/scroll) & endian-type

pub enum Endian {
    Little,
    Big,
}

fn scrlReadU16(buf: []const u8, off: usize, endian: Endian): u16 {
    const b0: u16 = buf[off] as u16;
    const b1: u16 = buf[off + 1] as u16;
    if (endian == Endian.Little) {
        return b0 | (b1 << 8);
    }
    return (b0 << 8) | b1;
}

fn scrlReadU32(buf: []const u8, off: usize, endian: Endian): u32 {
    const b0: u32 = buf[off] as u32;
    const b1: u32 = buf[off + 1] as u32;
    const b2: u32 = buf[off + 2] as u32;
    const b3: u32 = buf[off + 3] as u32;
    if (endian == Endian.Little) {
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn scrlReadU64(buf: []const u8, off: usize, endian: Endian): u64 {
    if (endian == Endian.Little) {
        const lo: u64 = scrlReadU32(buf, off, Endian.Little) as u64;
        const hi: u64 = scrlReadU32(buf, off + 4, Endian.Little) as u64;
        return lo | (hi << 32);
    }
    const hi: u64 = scrlReadU32(buf, off, Endian.Big) as u64;
    const lo: u64 = scrlReadU32(buf, off + 4, Endian.Big) as u64;
    return (hi << 32) | lo;
}

fn scrlWriteU16(buf: []u8, off: usize, v: u16, endian: Endian) {
    if (endian == Endian.Little) {
        buf[off] = (v & 0xFF) as u8;
        buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    } else {
        buf[off] = ((v >> 8) & 0xFF) as u8;
        buf[off + 1] = (v & 0xFF) as u8;
    }
}

fn scrlWriteU32(buf: []u8, off: usize, v: u32, endian: Endian) {
    if (endian == Endian.Little) {
        buf[off] = (v & 0xFF) as u8;
        buf[off + 1] = ((v >> 8) & 0xFF) as u8;
        buf[off + 2] = ((v >> 16) & 0xFF) as u8;
        buf[off + 3] = ((v >> 24) & 0xFF) as u8;
    } else {
        buf[off] = ((v >> 24) & 0xFF) as u8;
        buf[off + 1] = ((v >> 16) & 0xFF) as u8;
        buf[off + 2] = ((v >> 8) & 0xFF) as u8;
        buf[off + 3] = (v & 0xFF) as u8;
    }
}

fn scrlWriteU64(buf: []u8, off: usize, v: u64, endian: Endian) {
    const lo: u32 = (v & 0xFFFFFFFF) as u32;
    const hi: u32 = ((v >> 32) & 0xFFFFFFFF) as u32;
    if (endian == Endian.Little) {
        scrlWriteU32(buf, off, lo, Endian.Little);
        scrlWriteU32(buf, off + 4, hi, Endian.Little);
    } else {
        scrlWriteU32(buf, off, hi, Endian.Big);
        scrlWriteU32(buf, off + 4, lo, Endian.Big);
    }
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
        if (_.offset + 2 > _.buf.len) { return null; }
        const v = scrlReadU16(_.buf, _.offset, endian);
        _.offset += 2;
        return v;
    }

    fn readU32(*_, endian: Endian): u32? {
        if (_.offset + 4 > _.buf.len) { return null; }
        const v = scrlReadU32(_.buf, _.offset, endian);
        _.offset += 4;
        return v;
    }

    fn readU64(*_, endian: Endian): u64? {
        if (_.offset + 8 > _.buf.len) { return null; }
        const v = scrlReadU64(_.buf, _.offset, endian);
        _.offset += 8;
        return v;
    }

    fn readI32(*_, endian: Endian): i32? {
        if (_.offset + 4 > _.buf.len) { return null; }
        const u = scrlReadU32(_.buf, _.offset, endian);
        _.offset += 4;
        const v = @as(i32, @bitCast(u));
        return v;
    }

    fn readCstr(*_): string? {
        let i: usize = _.offset;
        while (i < _.buf.len) {
            if (_.buf[i] == 0) {
                const start = _.offset;
                _.offset = i + 1;
                return _.buf[start..i];
            }
            i += 1;
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
        if (_.offset + 2 > _.buf.len) { return false; }
        scrlWriteU16(_.buf, _.offset, v, endian);
        _.offset += 2;
        return true;
    }

    fn writeU32(*_, v: u32, endian: Endian): bool {
        if (_.offset + 4 > _.buf.len) { return false; }
        scrlWriteU32(_.buf, _.offset, v, endian);
        _.offset += 4;
        return true;
    }

    fn writeU64(*_, v: u64, endian: Endian): bool {
        if (_.offset + 8 > _.buf.len) { return false; }
        scrlWriteU64(_.buf, _.offset, v, endian);
        _.offset += 8;
        return true;
    }
}
