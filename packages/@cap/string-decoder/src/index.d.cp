// Type definitions for CP

export enum DecoderEncoding {
    Utf8,
    Latin1,
    Utf16Le
}

export fn sdecUtf8ExpectedLen(lead: u8): usize;
export fn sdecPushReplacement(out: []u8, pos: usize): usize;
export fn sdecPushCodePointUtf8(cp: u32, out: []u8, pos: usize): usize;
export class StringDecoder {
    encoding: DecoderEncoding,
    last_char: [4]u8,
    last_need: usize,
    last_total: usize,
    high_surrogate: u16,
    fn new(encoding: DecoderEncoding): StringDecoder;
    fn utf8(): StringDecoder;
    fn pendingBytes(): usize;
    fn write(_: auto, chunk: &[]u8, out: []u8): usize;
    fn writeUtf16Le(_: auto, chunk: &[]u8, out: []u8): usize;
    fn emitUtf16Unit(_: auto, unit: u16, out: []u8, pos: usize): usize;
    fn end(_: auto, out: []u8): usize;
}

