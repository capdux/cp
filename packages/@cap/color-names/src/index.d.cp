// Type definitions for CP

export class Rgb {
    r: u8,
    g: u8,
    b: u8,
    fn from(r: u8, g: u8, b: u8): Rgb;
}

export class Hsl {
    h: f64,
    s: f64,
    l: f64,
}

export class Hsv {
    h: f64,
    s: f64,
    v: f64,
}

export class NamedColor {
    name: string,
    r: u8,
    g: u8,
    b: u8,
}

export fn toLowerAscii(c: u8): u8;
export fn isWs(c: u8): bool;
export fn trimWs(s: string): string;
export fn cmpIgnoreCase(a: string, target: string): i32;
export fn clamp01(v: f64): f64;
export fn normalizeHue(h: f64): f64;
export fn roundToU8(v: f64): u8;
export fn absF64(x: f64): f64;
export fn cssColorCount(): usize;
export fn rgbFromCssName(name: string): Rgb?;
export fn cssNameFromRgb(c: Rgb): string?;
export fn rgbToHsl(c: Rgb): Hsl;
export fn hslToRgb(c: Hsl): Rgb;
export fn rgbToHsv(c: Rgb): Hsv;
export fn hsvToRgb(c: Hsv): Rgb;
export fn hexNibbleChar(v: u8): u8;
export fn parseHexNibble(c: u8): u8?;
export fn rgbToHex(c: Rgb, out: []u8): string;
export fn hexToRgb(hex: string): Rgb?;
export fn mixRgb(a: Rgb, b: Rgb, weight: f64): Rgb;
export fn lighten(c: Rgb, amount: f64): Rgb;
export fn darken(c: Rgb, amount: f64): Rgb;
