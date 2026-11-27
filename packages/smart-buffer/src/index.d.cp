// Type definitions for CP

import { readU16BE, readU16LE, readU32BE, readU32LE, writeU16BE, writeU16LE, writeU32BE, writeU32LE } from "cap:bin";
export fn sbufMin(a: usize, b: usize): usize;
export fn sbufW16BE(dst: []u8, off: usize, v: u16): bool;
export fn sbufW16LE(dst: []u8, off: usize, v: u16): bool;
export fn sbufW32BE(dst: []u8, off: usize, v: u32): bool;
export fn sbufW32LE(dst: []u8, off: usize, v: u32): bool;
export fn sbufR16BE(src: &[]u8, off: usize): u16?;
export fn sbufR16LE(src: &[]u8, off: usize): u16?;
export fn sbufR32BE(src: &[]u8, off: usize): u32?;
export fn sbufR32LE(src: &[]u8, off: usize): u32?;
export class SmartBuffer {
    buf: [1024]u8,
    read_offset: usize,
    write_offset: usize,
    fn new(): SmartBuffer;
    fn fromSlice(src: &[]u8): SmartBuffer;
    fn writeU8(_: auto, v: u8);
    fn writeU16BE(_: auto, v: u16);
    fn writeU16LE(_: auto, v: u16);
    fn writeU32BE(_: auto, v: u32);
    fn writeU32LE(_: auto, v: u32);
    fn writeString(_: auto, s: string);
    fn writeStringNT(_: auto, s: string);
    fn readU8(_: auto): u8;
    fn readU16BE(_: auto): u16;
    fn readU16LE(_: auto): u16;
    fn readU32BE(_: auto): u32;
    fn readU32LE(_: auto): u32;
    fn readString(_: auto, n: usize): string;
    fn remaining(): usize;
    fn len(): usize;
    fn rewind(_: auto);
}

