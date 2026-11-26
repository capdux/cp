// Type definitions for CP

export fn sbufMin(a: usize, b: usize): usize;
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

