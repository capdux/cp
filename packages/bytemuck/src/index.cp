import { readU16LE, readU32LE, readU32BE, readU64LE, writeU16LE, writeU32LE, writeU32BE, writeU64LE } from "cap:bin";

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
    return writeU16LE(out, 0, v);
}

pub fn readU16Le(buf: []u8): u16? {
    return readU16LE(buf, 0);
}

pub fn writeU32Le(v: u32, out: []u8): bool {
    return writeU32LE(out, 0, v);
}

pub fn readU32Le(buf: []u8): u32? {
    return readU32LE(buf, 0);
}

pub fn writeU32Be(v: u32, out: []u8): bool {
    return writeU32BE(out, 0, v);
}

pub fn readU32Be(buf: []u8): u32? {
    return readU32BE(buf, 0);
}

pub fn writeU64Le(v: u64, out: []u8): bool {
    return writeU64LE(out, 0, v);
}

pub fn readU64Le(buf: []u8): u64? {
    return readU64LE(buf, 0);
}

pub fn castU32SliceToBytesLe(src: []u32, dst: []u8): usize? {
    let needed = src.len * 4;
    if (dst.len < needed) { return null; }
    let i: usize = 0;
    while (i < src.len) {
        * = writeU32LE(dst, i * 4, src[i]);
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
        dst[i] = readU32LE(src, i * 4) ?? 0;
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
