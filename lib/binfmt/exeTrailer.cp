// Source: cp/lib/exe_format/trailer.cp (Standalone executable trailer append & zero-copy extraction)
import std from "std";
import { readU64Le, writeU64Le } from "./exeTypes.cp";

pub const TRAILER_FOOTER_SIZE: usize = 24;

pub struct StandaloneTrailer {
    payload: []const u8,
    payload_len: u64,
    checksum: u64,
    magic: [8]u8,
    total_len: usize,
}

pub fn computeChecksum64(data: []const u8): u64 {
    let hash: u64 = 0xCBF29CE484222325;
    for (b of data) {
        hash ^= (b as u64);
        hash = hash *% 0x100000001B3;
    }
    return hash;
}

pub fn appendStandaloneTrailer(alloc: std.mem.Allocator, exe_bytes: []const u8, payload: []const u8, magic8: [8]u8): []u8! {
    const total_len = exe_bytes.len + payload.len + TRAILER_FOOTER_SIZE;
    let out = alloc.alloc(u8, total_len)?;
    errdefer alloc.free(out);

    @memcpy(out[0..exe_bytes.len], exe_bytes);
    const payload_start = exe_bytes.len;
    @memcpy(out[payload_start .. payload_start + payload.len], payload);

    const footer_start = payload_start + payload.len;
    const payload_len: u64 = payload.len as u64;
    const checksum = computeChecksum64(payload);

    writeU64Le(out, footer_start, payload_len)?;
    writeU64Le(out, footer_start + 8, checksum)?;
    @memcpy(out[footer_start + 16 .. footer_start + 24], magic8[0..8]);

    return out;
}

pub fn extractStandaloneTrailer(bytes: []const u8, magic8: [8]u8): StandaloneTrailer! {
    if (bytes.len < TRAILER_FOOTER_SIZE) {
        return err.InvalidTrailer;
    }

    const footer_start = bytes.len - TRAILER_FOOTER_SIZE;
    const tail_magic = bytes[footer_start + 16 .. bytes.len];
    if (!std.mem.eql(u8, tail_magic, magic8[0..8])) {
        return err.InvalidMagic;
    }

    const payload_len = readU64Le(bytes, footer_start)?;
    const expected_checksum = readU64Le(bytes, footer_start + 8)?;

    if (payload_len > (footer_start as u64)) {
        return err.OutOfBounds;
    }

    const len_usize: usize = payload_len as usize;
    const payload_start = footer_start - len_usize;
    const payload = bytes[payload_start..footer_start];

    const actual_checksum = computeChecksum64(payload);
    if (actual_checksum != expected_checksum) {
        return err.ChecksumMismatch;
    }

    return StandaloneTrailer{
        payload: payload,
        payload_len: payload_len,
        checksum: expected_checksum,
        magic: magic8,
        total_len: len_usize + TRAILER_FOOTER_SIZE,
    };
}
