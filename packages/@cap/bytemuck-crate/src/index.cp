pub const POD_OK: u8 = 0;
pub const POD_ERR_SIZE_MISMATCH: u8 = 1;
pub const POD_ERR_ALIGNMENT: u8 = 2;

pub fn checkCastLayout(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): u8 {
    if (elem_size == 0 || (byte_len % elem_size) != 0) {
        return POD_ERR_SIZE_MISMATCH;
    }
    if (align_req > 1 && (addr % align_req) != 0) {
        return POD_ERR_ALIGNMENT;
    }
    return POD_OK;
}

pub fn writeU16Le(v: u16, out: []u8): bool {
    if (out.len < 2) { return false; }
    out[0] = (v & 255) as u8;
    out[1] = ((v >> 8) & 255) as u8;
    return true;
}

pub fn readU16Le(buf: []u8): u16? {
    if (buf.len < 2) { return null; }
    return (buf[0] as u16) | ((buf[1] as u16) << 8);
}

pub fn writeU32Le(v: u32, out: []u8): bool {
    if (out.len < 4) { return false; }
    out[0] = (v & 255) as u8;
    out[1] = ((v >> 8) & 255) as u8;
    out[2] = ((v >> 16) & 255) as u8;
    out[3] = ((v >> 24) & 255) as u8;
    return true;
}

pub fn readU32Le(buf: []u8): u32? {
    if (buf.len < 4) { return null; }
    return (buf[0] as u32) | ((buf[1] as u32) << 8) | ((buf[2] as u32) << 16) | ((buf[3] as u32) << 24);
}

pub fn writeU32Be(v: u32, out: []u8): bool {
    if (out.len < 4) { return false; }
    out[0] = ((v >> 24) & 255) as u8;
    out[1] = ((v >> 16) & 255) as u8;
    out[2] = ((v >> 8) & 255) as u8;
    out[3] = (v & 255) as u8;
    return true;
}

pub fn readU32Be(buf: []u8): u32? {
    if (buf.len < 4) { return null; }
    return ((buf[0] as u32) << 24) | ((buf[1] as u32) << 16) | ((buf[2] as u32) << 8) | (buf[3] as u32);
}

pub fn writeU64Le(v: u64, out: []u8): bool {
    if (out.len < 8) { return false; }
    let i: usize = 0;
    while (i < 8) {
        out[i] = ((v >> ((i * 8) as u64)) & 255) as u8;
        i = i + 1;
    }
    return true;
}

pub fn readU64Le(buf: []u8): u64? {
    if (buf.len < 8) { return null; }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < 8) {
        acc = acc | ((buf[i] as u64) << ((i * 8) as u64));
        i = i + 1;
    }
    return acc;
}

pub fn castU32SliceToBytesLe(src: []u32, dst: []u8): usize? {
    let needed = src.len * 4;
    if (dst.len < needed) { return null; }
    let i: usize = 0;
    while (i < src.len) {
        * = writeU32Le(src[i], dst[(i * 4)..(i * 4 + 4)]);
        i = i + 1;
    }
    return needed;
}

pub fn castBytesToU32SliceLe(src: []u8, dst: []u32): usize? {
    if ((src.len % 4) != 0) { return null; }
    let count = src.len / 4;
    if (dst.len < count) { return null; }
    let i: usize = 0;
    while (i < count) {
        dst[i] = readU32Le(src[(i * 4)..(i * 4 + 4)]) ?? 0;
        i = i + 1;
    }
    return count;
}

pub fn swapBytesU32(v: u32): u32 {
    let b0 = (v & 255) << 24;
    let b1 = ((v >> 8) & 255) << 16;
    let b2 = ((v >> 16) & 255) << 8;
    let b3 = (v >> 24) & 255;
    return b0 | b1 | b2 | b3;
}
