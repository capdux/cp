// Type definitions for CP

export fn peBitMask(byte: u8): u32;
export fn peHexUpper(nib: u8): u8;
export fn peHexVal(ch: u8): u8;
export class AsciiSet {
    bits: [4]u32,
    fn empty(): AsciiSet;
    fn add(_: auto, byte: u8);
    fn remove(_: auto, byte: u8);
    fn unionWith(other: AsciiSet): AsciiSet;
    fn shouldPercentEncode(byte: u8): bool;
}

export fn controlsSet(): AsciiSet;
export fn fragmentSet(): AsciiSet;
export fn querySet(): AsciiSet;
export fn pathSet(): AsciiSet;
export fn userinfoSet(): AsciiSet;
export fn componentSet(): AsciiSet;
export fn nonAlphanumericSet(): AsciiSet;
export fn percentEncodeLen(input: &[]u8, set: AsciiSet): usize;
export fn percentEncode(input: &[]u8, set: AsciiSet, out: []u8): usize;
export fn percentDecode(input: string, out: []u8): usize;
export fn percentDecodeStrict(input: string, out: []u8): i64;
export fn percentDecodePlus(input: string, out: []u8): usize;
