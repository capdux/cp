// Type definitions for CP

export const ADLER32_MOD: u32;
export const ADLER32_NMAX: usize;

export class Adler32 {
    s1: u32,
    s2: u32,
    fn init(): Adler32;
    fn new(): Adler32;
    fn fromValue(value: u32): Adler32;
    fn fromChecksum(value: u32): Adler32;
    fn update(_: auto, data: []const u8);
    fn writeSlice(_: auto, data: []const u8);
    fn writeStr(_: auto, s: string);
    fn updateByte(_: auto, b: u8);
    fn writeByte(_: auto, b: u8);
    fn roll(_: auto, window_len: usize, out_byte: u8, in_byte: u8);
    fn finalize(): u32;
    fn checksum(): u32;
    fn reset(_: auto);
}

export fn adler32(data: []const u8): u32;
export fn adler32Slice(data: []const u8): u32;
export fn adler32Str(s: string): u32;
export fn combineAdler32(adler1: u32, adler2: u32, len2: u64): u32;
export fn adler32Combine(adler1: u32, adler2: u32, len2: u64): u32;
