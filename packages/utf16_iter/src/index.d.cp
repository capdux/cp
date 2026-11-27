// Type definitions for CP

export fn u16iIsHighSurrogate(unit: u16): bool;
export fn u16iIsLowSurrogate(unit: u16): bool;
export fn u16iCombineSurrogates(high: u16, low: u16): u32;
export fn u16iEncodeUtf8(cp: u32, out: []u8, offset: usize): usize;
export class Utf16Chars {
    slice: &[]u16,
    pos: usize,
    end: usize,
    fn new(slice: &[]u16): Utf16Chars;
    fn isEmpty(): bool;
    fn asSlice(): &[]u16;
    fn next(_: auto): u32?;
    fn nextBack(_: auto): u32?;
}

export class Utf16Latin1 {
    bytes: &[]u8,
    pos: usize,
    end: usize,
    fn new(bytes: &[]u8): Utf16Latin1;
    fn isEmpty(): bool;
    fn asSlice(): &[]u8;
    fn next(_: auto): u16?;
    fn nextBack(_: auto): u16?;
    fn writeTo(out: []u16): usize;
    fn encode(bytes: &[]u8, out: []u16): usize;
}

export fn countUtf16Chars(slice: &[]u16): usize;
export fn utf16ToUtf8(slice: &[]u16, out: []u8): usize;
