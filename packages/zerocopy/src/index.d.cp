// Type definitions for CP

import { readU16Le, readU16Be, readU32Le, readU32Be, readU64Le, readU64Be, readF32Le, readF64Le, writeU16Le, writeU16Be, writeU32Le, writeU32Be, writeU64Le, writeU64Be, writeF32Le, writeF64Le, castU16SliceAsBytes, castU32SliceAsBytes, castU64SliceAsBytes, tryCastSliceBytesToU32, checkCastLayout, POD_OK } from "../../bytemuck/src/index.cp";
export class ByteCursor {
    pos: usize,
    total: usize,
    fn new(total_len: usize): ByteCursor;
    fn remaining(): usize;
    fn reset(_: auto);
    fn seek(_: auto, offset: usize): bool;
    fn peekU8(buf: &[]u8): u8?;
    fn nextU8(_: auto, buf: []u8): u8?;
    fn nextU16Le(_: auto, buf: []u8): u16?;
    fn nextU16Be(_: auto, buf: []u8): u16?;
    fn nextU32Le(_: auto, buf: []u8): u32?;
    fn nextU32Be(_: auto, buf: []u8): u32?;
    fn nextU64Le(_: auto, buf: []u8): u64?;
    fn nextU64Be(_: auto, buf: []u8): u64?;
    fn nextI32Le(_: auto, buf: []u8): i32?;
    fn nextF32Le(_: auto, buf: &[]u8): f32?;
    fn nextF64Le(_: auto, buf: &[]u8): f64?;
    fn nextBytes(_: auto, buf: &[]u8, n: usize): (&[]u8)?;
    fn skip(_: auto, n: usize): bool;
}

export class ByteWriterCursor {
    pos: usize,
    cap: usize,
    fn new(cap_len: usize): ByteWriterCursor;
    fn written(): usize;
    fn remaining(): usize;
    fn reset(_: auto);
    fn putU8(_: auto, out: []u8, v: u8): bool;
    fn putU16Le(_: auto, out: []u8, v: u16): bool;
    fn putU16Be(_: auto, out: []u8, v: u16): bool;
    fn putU32Le(_: auto, out: []u8, v: u32): bool;
    fn putU32Be(_: auto, out: []u8, v: u32): bool;
    fn putU64Le(_: auto, out: []u8, v: u64): bool;
    fn putU64Be(_: auto, out: []u8, v: u64): bool;
    fn putI32Le(_: auto, out: []u8, v: i32): bool;
    fn putF32Le(_: auto, out: []u8, v: f32): bool;
    fn putF64Le(_: auto, out: []u8, v: f64): bool;
    fn putBytes(_: auto, out: []u8, src: &[]u8): bool;
}

export fn isAlignedSlice(byte_len: usize, elem_size: usize, addr: usize, align_req: usize): bool;
export class U16LE {
    bytes: [2]u8,
    fn new(v: u16): U16LE;
    fn fromBytes(buf: &[]u8): U16LE?;
    fn get(): u16;
    fn set(_: auto, v: u16);
    fn toBytes(): [2]u8;
}

export class U16BE {
    bytes: [2]u8,
    fn new(v: u16): U16BE;
    fn fromBytes(buf: &[]u8): U16BE?;
    fn get(): u16;
    fn set(_: auto, v: u16);
    fn toBytes(): [2]u8;
}

export class U32LE {
    bytes: [4]u8,
    fn new(v: u32): U32LE;
    fn fromBytes(buf: &[]u8): U32LE?;
    fn get(): u32;
    fn set(_: auto, v: u32);
    fn toBytes(): [4]u8;
}

export class U32BE {
    bytes: [4]u8,
    fn new(v: u32): U32BE;
    fn fromBytes(buf: &[]u8): U32BE?;
    fn get(): u32;
    fn set(_: auto, v: u32);
    fn toBytes(): [4]u8;
}

export class U64LE {
    bytes: [8]u8,
    fn new(v: u64): U64LE;
    fn fromBytes(buf: &[]u8): U64LE?;
    fn get(): u64;
    fn set(_: auto, v: u64);
    fn toBytes(): [8]u8;
}

export class U64BE {
    bytes: [8]u8,
    fn new(v: u64): U64BE;
    fn fromBytes(buf: &[]u8): U64BE?;
    fn get(): u64;
    fn set(_: auto, v: u64);
    fn toBytes(): [8]u8;
}

export class SplitBytes {
    head: &[]u8,
    tail: &[]u8,
}

export class PrefixU16 {
    value: u16,
    rest: &[]u8,
}

export class PrefixU32 {
    value: u32,
    rest: &[]u8,
}

export class PrefixU64 {
    value: u64,
    rest: &[]u8,
}

export fn readFromPrefix(buf: &[]u8, n: usize): SplitBytes?;
export fn readFromSuffix(buf: &[]u8, n: usize): SplitBytes?;
export fn readU16LeFromPrefix(buf: &[]u8): PrefixU16?;
export fn readU16BeFromPrefix(buf: &[]u8): PrefixU16?;
export fn readU32LeFromPrefix(buf: &[]u8): PrefixU32?;
export fn readU32BeFromPrefix(buf: &[]u8): PrefixU32?;
export fn readU64LeFromPrefix(buf: &[]u8): PrefixU64?;
export fn readU64BeFromPrefix(buf: &[]u8): PrefixU64?;
export fn readU16LeFromSuffix(buf: &[]u8): PrefixU16?;
export fn readU32LeFromSuffix(buf: &[]u8): PrefixU32?;
export fn readU64LeFromSuffix(buf: &[]u8): PrefixU64?;
export fn writeToPrefix(dst: []u8, src: &[]u8): ([]u8)?;
export fn writeToSuffix(dst: []u8, src: &[]u8): ([]u8)?;
export fn refFromBytesU16(buf: &[]u8): u16?;
export fn refFromBytesU32(buf: &[]u8): u32?;
export fn refFromBytesU64(buf: &[]u8): u64?;
export fn refFromBytesSliceU32(buf: &[]u8): (&[]u32)?;
export fn sliceAsBytesU16(src: &[]u16): &[]u8;
export fn sliceAsBytesU32(src: &[]u32): &[]u8;
export fn sliceAsBytesU64(src: &[]u64): &[]u8;
