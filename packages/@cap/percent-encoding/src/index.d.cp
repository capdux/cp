// Type definitions for CP

export fn bit64(idx: u8): u64;
export class AsciiSet {
    lo: u64,
    hi: u64,
    fn empty(): AsciiSet;
    fn add(_: auto, b: u8);
    fn remove(_: auto, b: u8);
    fn contains(b: u8): bool;
}

export fn controlsSet(): AsciiSet;
export fn fragmentSet(): AsciiSet;
export fn querySet(): AsciiSet;
export fn pathSegmentSet(): AsciiSet;
export fn userinfoSet(): AsciiSet;
export fn isAsciiAlnum(b: u8): bool;
export fn nonAlphanumericSet(): AsciiSet;
export fn rfc3986UnreservedSet(): AsciiSet;
export fn hexUpperChar(nibble: u8): u8;
export fn hexVal(c: u8): i16;
export fn percentEncode(input: string, set: AsciiSet, out: []u8): string;
export fn percentDecode(input: string, out: []u8): string;
export fn percentDecodeStrict(input: string, out: []u8): string?;
export fn isFormUnescaped(b: u8): bool;
export fn formUrlEncode(input: string, out: []u8): string;
export fn formUrlDecode(input: string, out: []u8): string;
