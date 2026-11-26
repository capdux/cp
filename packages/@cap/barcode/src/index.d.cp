// Type definitions for CP

export class Barcode {
    modules: [MAX_MODULES]u8,
    len: usize,
    ok: bool,
    fn empty(): Barcode;
    fn pushBit(_: auto, bit: u8);
    fn pushBits(_: auto, pattern: string);
    fn pushWidths(_: auto, widths: string, start_black: bool);
    fn bitAt(idx: usize): u8;
    fn renderAscii(out: []u8): string;
    fn renderSvg(bar_width: usize, height: usize, quiet_zone: usize, out: []u8): string;
}

export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendUsize(out: []u8, pos: usize, v: usize): usize;
export fn isDigit(c: u8): bool;
export fn ean13Checksum(digits12: string): i8;
export fn isValidEan13(code: string): bool;
export fn upcAChecksum(digits11: string): i8;
export fn isValidUpcA(code: string): bool;
export fn eanLCode(d: u8): string;
export fn eanGCode(d: u8): string;
export fn eanRCode(d: u8): string;
export fn eanParityPattern(first_digit: u8): string;
export fn encodeEan13(code: string): Barcode;
export fn encodeUpcA(code: string): Barcode;
export fn code128Pattern(val: usize): string;
export fn encodeCode128B(text: string): Barcode;
export fn code39Pattern(c: u8): string;
export fn encodeCode39(text: string): Barcode;
