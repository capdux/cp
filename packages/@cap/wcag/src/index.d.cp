// Type definitions for CP

export class Rgb {
    r: u8,
    g: u8,
    b: u8,
}

export enum WcagLevel {
    Fail,
    AALarge,
    AA,
    AAA
}

export fn hexNibble(c: u8): i8;
export fn nibbleToHexLower(n: u8): u8;
export fn parseHex(hex: string): Rgb?;
export fn formatHex(c: Rgb, out: []u8): string;
export fn linearizeChannel(channel: u8): f64;
export fn relativeLuminance(c: Rgb): f64;
export fn contrastRatio(fg: Rgb, bg: Rgb): f64;
export fn contrastRatioHex(fg_hex: string, bg_hex: string): f64?;
export fn meetsAA(ratio: f64, large_text: bool): bool;
export fn meetsAAA(ratio: f64, large_text: bool): bool;
export fn gradeContrast(ratio: f64): WcagLevel;
export fn bestTextColor(bg: Rgb): Rgb;
export fn suggestAccessibleHex(bg_hex: string): string;
