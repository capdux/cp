// Type definitions for CP

import { readU16Le, readU32Le, readU32Be, readU64Le, writeU16Le, writeU32Le, writeU32Be, writeU64Le, checkCastLayout, POD_OK } from "../../bytemuck-crate/src/index.cp";
export class ByteCursor {
    pos: usize,
    total: usize,
    fn new(total_len: usize): ByteCursor;
    fn remaining(): usize;
    fn nextU8(_: auto, buf: []u8): u8?;
    fn nextU16Le(_: auto, buf: []u8): u16?;
    fn nextU32Le(_: auto, buf: []u8): u32?;
    fn nextU32Be(_: auto, buf: []u8): u32?;
    fn nextU64Le(_: auto, buf: []u8): u64?;
    fn skip(_: auto, n: usize): bool;
}

export class ByteWriterCursor {
    pos: usize,
    cap: usize,
    fn new(cap_len: usize): ByteWriterCursor;
    fn written(): usize;
    fn putU8(_: auto, out: []u8, v: u8): bool;
    fn putU16Le(_: auto, out: []u8, v: u16): bool;
    fn putU32Le(_: auto, out: []u8, v: u32): bool;
    fn putU32Be(_: auto, out: []u8, v: u32): bool;
    fn putU64Le(_: auto, out: []u8, v: u64): bool;
}

export fn isAlignedSlice(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): bool;
