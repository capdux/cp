// Type definitions for CP

import { readU16BE, readU16LE, readU32BE, readU32LE, readU64BE, readU64LE, writeU16BE, writeU16LE, writeU32BE, writeU32LE, writeU64BE, writeU64LE } from "cap:bin";
import { writeStr } from "cap:fmt";
export fn readU16Be(buf: string, off: usize): u16?;
export fn readU16Le(buf: string, off: usize): u16?;
export fn readU32Be(buf: string, off: usize): u32?;
export fn readU32Le(buf: string, off: usize): u32?;
export fn readU64Be(buf: string, off: usize): u64?;
export fn readU64Le(buf: string, off: usize): u64?;
export fn writeU16Be(out: []u8, off: usize, v: u16): bool;
export fn writeU16Le(out: []u8, off: usize, v: u16): bool;
export fn writeU32Be(out: []u8, off: usize, v: u32): bool;
export fn writeU32Le(out: []u8, off: usize, v: u32): bool;
export fn writeU64Be(out: []u8, off: usize, v: u64): bool;
export fn writeU64Le(out: []u8, off: usize, v: u64): bool;
export class ByteReader {
    data: string,
    pos: usize,
    fn init(data: string): ByteReader;
    fn remaining(): usize;
    fn atEnd(): bool;
    fn peekU8(): u8?;
    fn readU8(_: auto): u8?;
    fn readU16Be(_: auto): u16?;
    fn readU16Le(_: auto): u16?;
    fn readU32Be(_: auto): u32?;
    fn readU32Le(_: auto): u32?;
    fn readU64Be(_: auto): u64?;
    fn readU64Le(_: auto): u64?;
    fn readBytes(_: auto, n: usize): string?;
    fn skip(_: auto, n: usize): bool;
    fn readToEnd(_: auto): string;
}

export class ByteWriter {
    out: []u8,
    pos: usize,
    overflow: bool,
    fn init(out: []u8): ByteWriter;
    fn asSlice(): string;
    fn writeU8(_: auto, v: u8): bool;
    fn writeU16Be(_: auto, v: u16): bool;
    fn writeU16Le(_: auto, v: u16): bool;
    fn writeU32Be(_: auto, v: u32): bool;
    fn writeU32Le(_: auto, v: u32): bool;
    fn writeU64Be(_: auto, v: u64): bool;
    fn writeU64Le(_: auto, v: u64): bool;
    fn writeBytes(_: auto, s: string): bool;
}

