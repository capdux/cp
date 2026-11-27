// Type definitions for CP

import { writeHexByte, writeStr } from "cap:fmt";
export fn buildPrintableLut(): [256]u8;
export fn formatLine(data: &[]u8, base_offset: usize, out: []u8): usize;
export fn formatDump(data: &[]u8, out: []u8): usize;
export fn encodeHexSpaced(data: &[]u8, uppercase: bool, out: []u8): usize;
export fn firstDiffOffset(a: &[]u8, b: &[]u8): i64;
