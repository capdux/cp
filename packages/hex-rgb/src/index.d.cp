// Type definitions for CP

import { trim, hexVal } from "cap:ascii";
import { writeHexByte } from "cap:fmt";
export class Rgba {
    r: u8,
    g: u8,
    b: u8,
    a: u8,
    valid: bool,
}

export fn parseHexColor(input: string): Rgba;
export fn formatHex6(c: Rgba, uppercase: bool, out: []u8): usize;
export fn formatHex8(c: Rgba, uppercase: bool, out: []u8): usize;
export fn packRgba32(c: Rgba): u32;
export fn unpackRgba32(v: u32): Rgba;
export fn invertRgb(c: Rgba): Rgba;
export fn isDark(c: Rgba): bool;
export fn hcClampU8(v: f64): u8;
export fn blendOver(fg: Rgba, bg: Rgba): Rgba;
