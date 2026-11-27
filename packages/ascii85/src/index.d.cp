// Type definitions for CP

import { readU32BE, writeU32BE } from "cap:bin";
export fn buildA85DecLut(): [256]u8;
export fn a85IsSpace(c: u8): bool;
export fn maxEncodedLen(byte_len: usize, with_delimiters: bool): usize;
export fn encode(data: &[]u8, with_delimiters: bool, out: []u8): usize;
export fn decode(input: string, out: []u8): i64;
