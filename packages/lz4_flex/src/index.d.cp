// Type definitions for CP

import { readU16LE, readU32LE, writeU16LE, writeU32LE } from "cap:bin";
import { writeStr } from "cap:fmt";
export fn maxCompressedLen(input_len: usize): usize;
export fn hash4(buf: &[]u8, pos: usize): usize;
export fn match4(buf: &[]u8, a: usize, b: usize): bool;
export fn writeExtLen(dst: []u8, pos: usize, extra: usize): i64;
export fn compressBlock(src: &[]u8, dst: []u8): i64;
export fn decompressBlock(src: &[]u8, dst: []u8): i64;
export fn compressWithSize(src: &[]u8, dst: []u8): i64;
export fn decompressWithSize(src: &[]u8, dst: []u8): i64;
