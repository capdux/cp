// Type definitions for CP

export class Adler32 {
    s1: u32,
    s2: u32,
    fn init(): Adler32;
    fn fromValue(checksum: u32): Adler32;
    fn update(_: auto, data: &[]u8);
    fn updateByte(_: auto, b: u8);
    fn roll(_: auto, window_len: usize, out_byte: u8, in_byte: u8);
    fn finalize(): u32;
    fn reset(_: auto);
}

export fn adler32(data: &[]u8): u32;
export fn combineAdler32(adler1: u32, adler2: u32, len2: u64): u32;
