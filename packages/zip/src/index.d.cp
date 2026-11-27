// Type definitions for CP

import { hashBytes } from "../../crc32fast/src/index.cp";
import { readU16Le, readU32Le, writeU16Le, writeU32Le } from "../../bytemuck-crate/src/index.cp";
export class ZipLocalHeader {
    version_needed: u16,
    flags: u16,
    method: u16,
    crc32: u32,
    compressed_size: u32,
    uncompressed_size: u32,
    name_len: u16,
    extra_len: u16,
    data_offset: usize,
    valid: bool,
}

export class ZipEocd {
    disk_num: u16,
    total_entries: u16,
    cd_size: u32,
    cd_offset: u32,
    valid: bool,
}

export fn zipEmptyLocalHeader(): ZipLocalHeader;
export fn zipEmptyEocd(): ZipEocd;
export fn zipCopyStr(dst: []u8, offset: usize, s: string);
export fn zipCopyBytes(dst: []u8, offset: usize, src: []u8);
export fn writeStoredZipSingle(filename: string, payload: []u8, out: []u8): usize?;
export fn parseLocalHeader(buf: []u8, offset: usize): ZipLocalHeader;
export fn findAndParseEocd(buf: []u8): ZipEocd;
export fn verifyStoredEntry(buf: []u8, hdr: ZipLocalHeader): bool;
