// Type definitions for CP

import { hashBytes } from "../../crc32fast/src/index.cp";
import { adler32 } from "../../adler32/src/index.cp";
import { readU16Le, readU32Le, readU32Be, writeU16Le, writeU32Le, writeU32Be } from "../../bytemuck-crate/src/index.cp";
export class GzipHeader {
    method: u8,
    flags: u8,
    mtime: u32,
    os: u8,
    data_offset: usize,
    valid: bool,
}

export fn fl2EmptyGzipHeader(): GzipHeader;
export fn fl2StoredEncodedLen(payload_len: usize): usize;
export fn fl2EncodeStoredBlocks(payload: []u8, out: []u8, start: usize): usize;
export fn fl2DecodeStoredBlocks(stream: []u8, out: []u8): usize?;
export fn parseGzipHeader(frame: []u8): GzipHeader;
export fn encodeGzipStored(payload: []u8, mtime: u32, out: []u8): usize?;
export fn decodeGzipStored(frame: []u8, out: []u8): usize?;
export fn encodeZlibStored(payload: []u8, out: []u8): usize?;
export fn decodeZlibStored(frame: []u8, out: []u8): usize?;
