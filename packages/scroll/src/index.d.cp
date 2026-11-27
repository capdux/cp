// Type definitions for CP

import { readU16LE, readU16BE, readU32LE, readU32BE, readU64LE, readU64BE, writeU16LE, writeU16BE, writeU32LE, writeU32BE, writeU64LE, writeU64BE, findByteInBytes } from "cap:bin";
export enum Endian {
    Little,
    Big
}

export class PreadCursor {
    buf: &[]u8,
    offset: usize,
    fn new(buf: &[]u8): PreadCursor;
    fn remaining(): usize;
    fn readU8(_: auto): u8?;
    fn readU16(_: auto, endian: Endian): u16?;
    fn readU32(_: auto, endian: Endian): u32?;
    fn readU64(_: auto, endian: Endian): u64?;
    fn readI32(_: auto, endian: Endian): i32?;
    fn readCstr(_: auto): string?;
}

export class PwriteCursor {
    buf: []u8,
    offset: usize,
    fn new(buf: []u8): PwriteCursor;
    fn written(): usize;
    fn remaining(): usize;
    fn writeU8(_: auto, v: u8): bool;
    fn writeU16(_: auto, v: u16, endian: Endian): bool;
    fn writeU32(_: auto, v: u32, endian: Endian): bool;
    fn writeU64(_: auto, v: u64, endian: Endian): bool;
}

