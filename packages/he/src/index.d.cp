// Type definitions for CP

export class Utf8Step {
    cp: u32,
    width: usize,
}

export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushStr(out: []u8, pos: usize, s: string): usize;
export fn writeUtf8(out: []u8, pos: usize, raw_cp: u32): usize;
export fn decodeUtf8At(s: string, i: usize): Utf8Step;
export fn pushHexEntity(out: []u8, pos: usize, cp: u32): usize;
export fn strEq(a: string, b: string): bool;
export fn needsHtmlEscape(s: string): bool;
export fn escapeHtml(input: string, out: []u8): string;
export fn encodeNonAscii(input: string, out: []u8): string;
export fn lookupNamedEntity(name: string): u32;
export fn hexDigit(c: u8): i8;
export fn decodeHtmlEntities(input: string, out: []u8): string;
