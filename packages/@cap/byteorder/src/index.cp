// @cap/byteorder — Zero-heap Endian Codecs & Bounds-Checked Untrusted Binary Reader/Writer
// Clean-room MIT implementation in pure CP (inspired by Rust `byteorder` [993M] + `untrusted` [1.02B]).

// ============================================================================
// Stateless Big-Endian / Little-Endian Slice Codecs
// ============================================================================

pub fn readU16Be(buf: string, off: usize) -> u16? {
    if (off + 2 > buf.len) { return null; }
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

pub fn readU16Le(buf: string, off: usize) -> u16? {
    if (off + 2 > buf.len) { return null; }
    return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
}

pub fn readU32Be(buf: string, off: usize) -> u32? {
    if (off + 4 > buf.len) { return null; }
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

pub fn readU32Le(buf: string, off: usize) -> u32? {
    if (off + 4 > buf.len) { return null; }
    return (buf[off] as u32) |
        ((buf[off + 1] as u32) << 8) |
        ((buf[off + 2] as u32) << 16) |
        ((buf[off + 3] as u32) << 24);
}

pub fn readU64Be(buf: string, off: usize) -> u64? {
    if (off + 8 > buf.len) { return null; }
    return ((buf[off] as u64) << 56) |
        ((buf[off + 1] as u64) << 48) |
        ((buf[off + 2] as u64) << 40) |
        ((buf[off + 3] as u64) << 32) |
        ((buf[off + 4] as u64) << 24) |
        ((buf[off + 5] as u64) << 16) |
        ((buf[off + 6] as u64) << 8) |
        (buf[off + 7] as u64);
}

pub fn readU64Le(buf: string, off: usize) -> u64? {
    if (off + 8 > buf.len) { return null; }
    return (buf[off] as u64) |
        ((buf[off + 1] as u64) << 8) |
        ((buf[off + 2] as u64) << 16) |
        ((buf[off + 3] as u64) << 24) |
        ((buf[off + 4] as u64) << 32) |
        ((buf[off + 5] as u64) << 40) |
        ((buf[off + 6] as u64) << 48) |
        ((buf[off + 7] as u64) << 56);
}

pub fn writeU16Be(out: []u8, off: usize, v: u16) -> bool {
    if (off + 2 > out.len) { return false; }
    out[off] = ((v >> 8) & 0xFF) as u8;
    out[off + 1] = (v & 0xFF) as u8;
    return true;
}

pub fn writeU16Le(out: []u8, off: usize, v: u16) -> bool {
    if (off + 2 > out.len) { return false; }
    out[off] = (v & 0xFF) as u8;
    out[off + 1] = ((v >> 8) & 0xFF) as u8;
    return true;
}

pub fn writeU32Be(out: []u8, off: usize, v: u32) -> bool {
    if (off + 4 > out.len) { return false; }
    out[off] = ((v >> 24) & 0xFF) as u8;
    out[off + 1] = ((v >> 16) & 0xFF) as u8;
    out[off + 2] = ((v >> 8) & 0xFF) as u8;
    out[off + 3] = (v & 0xFF) as u8;
    return true;
}

pub fn writeU32Le(out: []u8, off: usize, v: u32) -> bool {
    if (off + 4 > out.len) { return false; }
    out[off] = (v & 0xFF) as u8;
    out[off + 1] = ((v >> 8) & 0xFF) as u8;
    out[off + 2] = ((v >> 16) & 0xFF) as u8;
    out[off + 3] = ((v >> 24) & 0xFF) as u8;
    return true;
}

pub fn writeU64Be(out: []u8, off: usize, v: u64) -> bool {
    if (off + 8 > out.len) { return false; }
    out[off] = ((v >> 56) & 0xFF) as u8;
    out[off + 1] = ((v >> 48) & 0xFF) as u8;
    out[off + 2] = ((v >> 40) & 0xFF) as u8;
    out[off + 3] = ((v >> 32) & 0xFF) as u8;
    out[off + 4] = ((v >> 24) & 0xFF) as u8;
    out[off + 5] = ((v >> 16) & 0xFF) as u8;
    out[off + 6] = ((v >> 8) & 0xFF) as u8;
    out[off + 7] = (v & 0xFF) as u8;
    return true;
}

pub fn writeU64Le(out: []u8, off: usize, v: u64) -> bool {
    if (off + 8 > out.len) { return false; }
    out[off] = (v & 0xFF) as u8;
    out[off + 1] = ((v >> 8) & 0xFF) as u8;
    out[off + 2] = ((v >> 16) & 0xFF) as u8;
    out[off + 3] = ((v >> 24) & 0xFF) as u8;
    out[off + 4] = ((v >> 32) & 0xFF) as u8;
    out[off + 5] = ((v >> 40) & 0xFF) as u8;
    out[off + 6] = ((v >> 48) & 0xFF) as u8;
    out[off + 7] = ((v >> 56) & 0xFF) as u8;
    return true;
}

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
        if (_.pos + 2 > _.data.len) { return null; }
        const v: u16 = ((_.data[_.pos] as u16) << 8) | (_.data[_.pos + 1] as u16);
        _.pos += 2;
        return v;
    }

    pub fn readU16Le(*_) -> u16? {
        if (_.pos + 2 > _.data.len) { return null; }
        const v: u16 = (_.data[_.pos] as u16) | ((_.data[_.pos + 1] as u16) << 8);
        _.pos += 2;
        return v;
    }

    pub fn readU32Be(*_) -> u32? {
        if (_.pos + 4 > _.data.len) { return null; }
        const v: u32 = ((_.data[_.pos] as u32) << 24) |
            ((_.data[_.pos + 1] as u32) << 16) |
            ((_.data[_.pos + 2] as u32) << 8) |
            (_.data[_.pos + 3] as u32);
        _.pos += 4;
        return v;
    }

    pub fn readU32Le(*_) -> u32? {
        if (_.pos + 4 > _.data.len) { return null; }
        const v: u32 = (_.data[_.pos] as u32) |
            ((_.data[_.pos + 1] as u32) << 8) |
            ((_.data[_.pos + 2] as u32) << 16) |
            ((_.data[_.pos + 3] as u32) << 24);
        _.pos += 4;
        return v;
    }

    pub fn readU64Be(*_) -> u64? {
        if (_.pos + 8 > _.data.len) { return null; }
        const v: u64 = ((_.data[_.pos] as u64) << 56) |
            ((_.data[_.pos + 1] as u64) << 48) |
            ((_.data[_.pos + 2] as u64) << 40) |
            ((_.data[_.pos + 3] as u64) << 32) |
            ((_.data[_.pos + 4] as u64) << 24) |
            ((_.data[_.pos + 5] as u64) << 16) |
            ((_.data[_.pos + 6] as u64) << 8) |
            (_.data[_.pos + 7] as u64);
        _.pos += 8;
        return v;
    }

    pub fn readU64Le(*_) -> u64? {
        if (_.pos + 8 > _.data.len) { return null; }
        const v: u64 = (_.data[_.pos] as u64) |
            ((_.data[_.pos + 1] as u64) << 8) |
            ((_.data[_.pos + 2] as u64) << 16) |
            ((_.data[_.pos + 3] as u64) << 24) |
            ((_.data[_.pos + 4] as u64) << 32) |
            ((_.data[_.pos + 5] as u64) << 40) |
            ((_.data[_.pos + 6] as u64) << 48) |
            ((_.data[_.pos + 7] as u64) << 56);
        _.pos += 8;
        return v;
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
        if (_.pos + 2 > _.out.len) { _.overflow = true; return false; }
        _.out[_.pos] = ((v >> 8) & 0xFF) as u8;
        _.out[_.pos + 1] = (v & 0xFF) as u8;
        _.pos += 2;
        return true;
    }

    pub fn writeU16Le(*_, v: u16) -> bool {
        if (_.pos + 2 > _.out.len) { _.overflow = true; return false; }
        _.out[_.pos] = (v & 0xFF) as u8;
        _.out[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.pos += 2;
        return true;
    }

    pub fn writeU32Be(*_, v: u32) -> bool {
        if (_.pos + 4 > _.out.len) { _.overflow = true; return false; }
        _.out[_.pos] = ((v >> 24) & 0xFF) as u8;
        _.out[_.pos + 1] = ((v >> 16) & 0xFF) as u8;
        _.out[_.pos + 2] = ((v >> 8) & 0xFF) as u8;
        _.out[_.pos + 3] = (v & 0xFF) as u8;
        _.pos += 4;
        return true;
    }

    pub fn writeU32Le(*_, v: u32) -> bool {
        if (_.pos + 4 > _.out.len) { _.overflow = true; return false; }
        _.out[_.pos] = (v & 0xFF) as u8;
        _.out[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.out[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
        _.out[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        _.pos += 4;
        return true;
    }

    pub fn writeU64Be(*_, v: u64) -> bool {
        if (_.pos + 8 > _.out.len) { _.overflow = true; return false; }
        _.out[_.pos] = ((v >> 56) & 0xFF) as u8;
        _.out[_.pos + 1] = ((v >> 48) & 0xFF) as u8;
        _.out[_.pos + 2] = ((v >> 40) & 0xFF) as u8;
        _.out[_.pos + 3] = ((v >> 32) & 0xFF) as u8;
        _.out[_.pos + 4] = ((v >> 24) & 0xFF) as u8;
        _.out[_.pos + 5] = ((v >> 16) & 0xFF) as u8;
        _.out[_.pos + 6] = ((v >> 8) & 0xFF) as u8;
        _.out[_.pos + 7] = (v & 0xFF) as u8;
        _.pos += 8;
        return true;
    }

    pub fn writeU64Le(*_, v: u64) -> bool {
        if (_.pos + 8 > _.out.len) { _.overflow = true; return false; }
        _.out[_.pos] = (v & 0xFF) as u8;
        _.out[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.out[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
        _.out[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        _.out[_.pos + 4] = ((v >> 32) & 0xFF) as u8;
        _.out[_.pos + 5] = ((v >> 40) & 0xFF) as u8;
        _.out[_.pos + 6] = ((v >> 48) & 0xFF) as u8;
        _.out[_.pos + 7] = ((v >> 56) & 0xFF) as u8;
        _.pos += 8;
        return true;
    }

    pub fn writeBytes(*_, s: string) -> bool {
        if (s.len > _.out.len - _.pos) { _.overflow = true; return false; }
        let i: usize = 0;
        while (i < s.len) {
            _.out[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }
}
