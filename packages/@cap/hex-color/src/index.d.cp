// Type definitions for CP

export class Rgba {
    r: u8,
    g: u8,
    b: u8,
    a: u8,
    valid: bool,
}

export fn hcIsSpace(c: u8): bool;
export fn hcTrim(s: &[]u8): &[]u8;
export fn hcHexVal(c: u8): u8;
export fn hcHexChar(nibble: u8, uppercase: bool): u8;
export fn hcWriteByteHex(out: []u8, pos: usize, val: u8, uppercase: bool);
export fn parseHexColor(input: string): Rgba;
export fn formatHex6(c: Rgba, uppercase: bool, out: []u8): usize;
export fn formatHex8(c: Rgba, uppercase: bool, out: []u8): usize;
export fn packRgba32(c: Rgba): u32;
export fn unpackRgba32(v: u32): Rgba;
export fn invertRgb(c: Rgba): Rgba;
export fn isDark(c: Rgba): bool;
export fn hcClampU8(v: f64): u8;
export fn blendOver(fg: Rgba, bg: Rgba): Rgba;
