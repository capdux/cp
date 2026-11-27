// @cap/bytemuck — Safe POD byte casting, alignment checks & unaligned reads
// Modeled on crates.io `bytemuck` (Pod / Zeroable / cast_slice / bytes_of / from_bytes / pod_read_unaligned).

import std from "std";
import {
    readU16LE, readU16BE, readU32LE, readU32BE, readU64LE, readU64BE,
    writeU16LE, writeU16BE, writeU32LE, writeU32BE, writeU64LE, writeU64BE,
} from "cap:bin";

pub const POD_OK: u8 = 0;
pub const POD_ERR_SIZE_MISMATCH: u8 = 1;
pub const POD_ERR_ALIGNMENT: u8 = 2;
pub const POD_ERR_OUTPUT_SLICE_TOO_SMALL: u8 = 3;
pub const POD_ERR_TARGET_ALIGNMENT_GREATER: u8 = 4;

/// Result wrapper for `tryCastSlice*` operations reporting exact `POD_*` error code.
pub struct CastSliceResult {
    pub count: usize,
    pub err_code: u8,

    pub fn isOk(): bool {
        return _.err_code == POD_OK;
    }
}

pub fn checkCastLayout(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): u8 {
    if (elem_size == 0 || (byte_len % elem_size) != 0) {
        return POD_ERR_SIZE_MISMATCH;
    }
    if (align_req > 1 && (addr % align_req) != 0) {
        return POD_ERR_ALIGNMENT;
    }
    return POD_OK;
}

/// Returns `true` if `addr` satisfies `align_req` alignment.
pub fn isAligned(addr: usize, align_req: usize): bool {
    if (align_req <= 1) { return true; }
    return (addr % align_req) == 0;
}

// --- Zeroable helpers ---

/// Zeroes all bytes in `buf` (equivalent to `bytemuck::Zeroable::zeroed` on a byte slice).
pub fn fillZeroed(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) : (i += 1) {
        buf[i] = 0;
    }
}

pub fn zeroedU8(): u8 { return 0; }
pub fn zeroedU16(): u16 { return 0; }
pub fn zeroedU32(): u32 { return 0; }
pub fn zeroedU64(): u64 { return 0; }
pub fn zeroedI32(): i32 { return 0; }
pub fn zeroedI64(): i64 { return 0; }
pub fn zeroedF32(): f32 { return 0.0; }
pub fn zeroedF64(): f64 { return 0.0; }

// --- Scalar LE/BE read/write ---

pub fn writeU16Le(v: u16, out: []u8): bool {
    return writeU16LE(out, 0, v);
}

pub fn readU16Le(buf: []u8): u16? {
    return readU16LE(buf, 0);
}

pub fn writeU16Be(v: u16, out: []u8): bool {
    return writeU16BE(out, 0, v);
}

pub fn readU16Be(buf: []u8): u16? {
    return readU16BE(buf, 0);
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

pub fn writeU64Be(v: u64, out: []u8): bool {
    return writeU64BE(out, 0, v);
}

pub fn readU64Be(buf: []u8): u64? {
    return readU64BE(buf, 0);
}

pub fn writeF32Le(v: f32, out: []u8): bool {
    const bits: u32 = @bitCast(v);
    return writeU32LE(out, 0, bits);
}

pub fn readF32Le(buf: []const u8): f32? {
    if (buf.len < 4) { return null; }
    const b0: u32 = buf[0] as u32;
    const b1: u32 = (buf[1] as u32) << 8;
    const b2: u32 = (buf[2] as u32) << 16;
    const b3: u32 = (buf[3] as u32) << 24;
    const bits: u32 = b0 | b1 | b2 | b3;
    const res: f32 = @bitCast(bits);
    return res;
}

pub fn writeF64Le(v: f64, out: []u8): bool {
    const bits: u64 = @bitCast(v);
    return writeU64LE(out, 0, bits);
}

pub fn readF64Le(buf: []const u8): f64? {
    if (buf.len < 8) { return null; }
    let bits: u64 = 0;
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        bits = bits | ((buf[i] as u64) << ((i * 8) as u6));
    }
    const res: f64 = @bitCast(bits);
    return res;
}

// --- bytemuck::bytes_of equivalents ---

pub fn bytesOfU16(v: u16): [2]u8 {
    let out: [2]u8 = [0; 2];
    out[0] = (v & 0xFF) as u8;
    out[1] = ((v >> 8) & 0xFF) as u8;
    return out;
}

pub fn bytesOfU32(v: u32): [4]u8 {
    let out: [4]u8 = [0; 4];
    out[0] = (v & 0xFF) as u8;
    out[1] = ((v >> 8) & 0xFF) as u8;
    out[2] = ((v >> 16) & 0xFF) as u8;
    out[3] = ((v >> 24) & 0xFF) as u8;
    return out;
}

pub fn bytesOfU64(v: u64): [8]u8 {
    let out: [8]u8 = [0; 8];
    out[0] = (v & 0xFF) as u8;
    out[1] = ((v >> 8) & 0xFF) as u8;
    out[2] = ((v >> 16) & 0xFF) as u8;
    out[3] = ((v >> 24) & 0xFF) as u8;
    out[4] = ((v >> 32) & 0xFF) as u8;
    out[5] = ((v >> 40) & 0xFF) as u8;
    out[6] = ((v >> 48) & 0xFF) as u8;
    out[7] = ((v >> 56) & 0xFF) as u8;
    return out;
}

pub fn bytesOfF32(v: f32): [4]u8 {
    const bits: u32 = @bitCast(v);
    return bytesOfU32(bits);
}

pub fn bytesOfF64(v: f64): [8]u8 {
    const bits: u64 = @bitCast(v);
    return bytesOfU64(bits);
}

// --- bytemuck::from_bytes & pod_read_unaligned equivalents ---

/// Exact-length checked byte-to-u16 conversion (mirrors `bytemuck::try_from_bytes`).
pub fn fromBytesU16(buf: []const u8): u16? {
    if (buf.len != 2) { return null; }
    return podReadUnalignedU16(buf);
}

/// Exact-length checked byte-to-u32 conversion (mirrors `bytemuck::try_from_bytes`).
pub fn fromBytesU32(buf: []const u8): u32? {
    if (buf.len != 4) { return null; }
    return podReadUnalignedU32(buf);
}

/// Exact-length checked byte-to-u64 conversion (mirrors `bytemuck::try_from_bytes`).
pub fn fromBytesU64(buf: []const u8): u64? {
    if (buf.len != 8) { return null; }
    return podReadUnalignedU64(buf);
}

/// Exact-length checked byte-to-f32 conversion.
pub fn fromBytesF32(buf: []const u8): f32? {
    if (buf.len != 4) { return null; }
    return readF32Le(buf);
}

/// Exact-length checked byte-to-f64 conversion.
pub fn fromBytesF64(buf: []const u8): f64? {
    if (buf.len != 8) { return null; }
    return readF64Le(buf);
}

/// Safe unaligned POD read of `u16` from any byte slice of length >= 2 (mirrors `pod_read_unaligned`).
pub fn podReadUnalignedU16(buf: []const u8): u16? {
    if (buf.len < 2) { return null; }
    return (buf[0] as u16) | ((buf[1] as u16) << 8);
}

/// Safe unaligned POD read of `u32` from any byte slice of length >= 4.
pub fn podReadUnalignedU32(buf: []const u8): u32? {
    if (buf.len < 4) { return null; }
    return (buf[0] as u32)
        | ((buf[1] as u32) << 8)
        | ((buf[2] as u32) << 16)
        | ((buf[3] as u32) << 24);
}

/// Safe unaligned POD read of `u64` from any byte slice of length >= 8.
pub fn podReadUnalignedU64(buf: []const u8): u64? {
    if (buf.len < 8) { return null; }
    let v: u64 = 0;
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        v = v | ((buf[i] as u64) << ((i * 8) as u6));
    }
    return v;
}

/// Safe unaligned POD read of `i32` from any byte slice of length >= 4.
pub fn podReadUnalignedI32(buf: []const u8): i32? {
    const raw = podReadUnalignedU32(buf);
    if (raw == null) { return null; }
    const res: i32 = @bitCast(raw.?);
    return res;
}

/// Safe unaligned POD read of `f32` from any byte slice of length >= 4.
pub fn podReadUnalignedF32(buf: []const u8): f32? {
    return readF32Le(buf);
}

/// Safe unaligned POD read of `f64` from any byte slice of length >= 8.
pub fn podReadUnalignedF64(buf: []const u8): f64? {
    return readF64Le(buf);
}

// --- Slice casting (castSlice / tryCastSlice) ---

pub fn castU16SliceToBytesLe(src: []const u16, dst: []u8): usize? {
    let needed = src.len * 2;
    if (dst.len < needed) { return null; }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        * = writeU16LE(dst, i * 2, src[i]);
    }
    return needed;
}

pub fn castBytesToU16SliceLe(src: []const u8, dst: []u16): usize? {
    if ((src.len % 2) != 0) { return null; }
    let count = src.len / 2;
    if (dst.len < count) { return null; }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        dst[i] = podReadUnalignedU16(src[i * 2..src.len]) ?? 0;
    }
    return count;
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

pub fn castU64SliceToBytesLe(src: []const u64, dst: []u8): usize? {
    let needed = src.len * 8;
    if (dst.len < needed) { return null; }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        * = writeU64LE(dst, i * 8, src[i]);
    }
    return needed;
}

pub fn castBytesToU64SliceLe(src: []const u8, dst: []u64): usize? {
    if ((src.len % 8) != 0) { return null; }
    let count = src.len / 8;
    if (dst.len < count) { return null; }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        dst[i] = podReadUnalignedU64(src[i * 8..src.len]) ?? 0;
    }
    return count;
}

/// Checked slice cast from `[]const u8` to `[]u16` returning structured `CastSliceResult`.
pub fn tryCastBytesToU16Slice(src: []const u8, dst: []u16): CastSliceResult {
    if ((src.len % 2) != 0) {
        return CastSliceResult { count: 0, err_code: POD_ERR_SIZE_MISMATCH };
    }
    let count = src.len / 2;
    if (dst.len < count) {
        return CastSliceResult { count: 0, err_code: POD_ERR_OUTPUT_SLICE_TOO_SMALL };
    }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        dst[i] = podReadUnalignedU16(src[i * 2..src.len]) ?? 0;
    }
    return CastSliceResult { count: count, err_code: POD_OK };
}

/// Checked slice cast from `[]const u8` to `[]u32` returning structured `CastSliceResult`.
pub fn tryCastBytesToU32Slice(src: []const u8, dst: []u32): CastSliceResult {
    if ((src.len % 4) != 0) {
        return CastSliceResult { count: 0, err_code: POD_ERR_SIZE_MISMATCH };
    }
    let count = src.len / 4;
    if (dst.len < count) {
        return CastSliceResult { count: 0, err_code: POD_ERR_OUTPUT_SLICE_TOO_SMALL };
    }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        dst[i] = podReadUnalignedU32(src[i * 4..src.len]) ?? 0;
    }
    return CastSliceResult { count: count, err_code: POD_OK };
}

/// Checked slice cast from `[]const u8` to `[]u64` returning structured `CastSliceResult`.
pub fn tryCastBytesToU64Slice(src: []const u8, dst: []u64): CastSliceResult {
    if ((src.len % 8) != 0) {
        return CastSliceResult { count: 0, err_code: POD_ERR_SIZE_MISMATCH };
    }
    let count = src.len / 8;
    if (dst.len < count) {
        return CastSliceResult { count: 0, err_code: POD_ERR_OUTPUT_SLICE_TOO_SMALL };
    }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        dst[i] = podReadUnalignedU64(src[i * 8..src.len]) ?? 0;
    }
    return CastSliceResult { count: count, err_code: POD_OK };
}

/// Zero-copy view of a `[]const u32` slice as raw bytes (mirrors `bytemuck::cast_slice`).
pub fn castU32SliceAsBytes(src: []const u32): []const u8 {
    return std.mem.sliceAsBytes(src);
}

/// Zero-copy view of a `[]const u16` slice as raw bytes.
pub fn castU16SliceAsBytes(src: []const u16): []const u8 {
    return std.mem.sliceAsBytes(src);
}

/// Zero-copy view of a `[]const u64` slice as raw bytes.
pub fn castU64SliceAsBytes(src: []const u64): []const u8 {
    return std.mem.sliceAsBytes(src);
}

/// Zero-copy aligned `tryCastSlice` from `[]const u8` to `[]const u32`.
/// Returns `null` if length is not a multiple of 4 or pointer is not 4-byte aligned.
pub fn tryCastSliceBytesToU32(src: []const u8): ([]const u32)? {
    unsafe {
        const addr = @intFromPtr(src.ptr);
        if (checkCastLayout(src.len, 4, addr, 4) != POD_OK) {
            return null;
        }
        const aligned_ptr: [*]const u32 = @ptrCast(@alignCast(src.ptr));
        return aligned_ptr[0..(src.len / 4)];
    }
}

// --- Endian byte swapping ---

pub fn swapBytesU16(v: u16): u16 {
    return ((v & 0xFF) << 8) | ((v >> 8) & 0xFF);
}

pub fn swapBytesU32(v: u32): u32 {
    let b0 = (v & 255) << 24;
    let b1 = ((v >> 8) & 255) << 16;
    let b2 = ((v >> 16) & 255) << 8;
    let b3 = (v >> 24) & 255;
    return b0 | b1 | b2 | b3;
}

pub fn swapBytesU64(v: u64): u64 {
    let lo: u64 = swapBytesU32((v & 0xFFFFFFFF) as u32) as u64;
    let hi: u64 = swapBytesU32(((v >> 32) & 0xFFFFFFFF) as u32) as u64;
    return (lo << 32) | hi;
}
