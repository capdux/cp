// Type definitions for CP

import { RgbColor, lookupColor, reverseLookupColor } from "../../color-name/src/index.cp";
export class Rgb {
    r: u8,
    g: u8,
    b: u8,
}

export class Hsv {
    h: f64,
    s: f64,
    v: f64,
}

export class Hwb {
    h: f64,
    w: f64,
    b: f64,
}

export class Hsl {
    h: f64,
    s: f64,
    l: f64,
}

export class Cmyk {
    c: f64,
    m: f64,
    y: f64,
    k: f64,
}

export fn ccAbs(x: f64): f64;
export fn ccClamp01(x: f64): f64;
export fn ccNormHue(h: f64): f64;
export fn ccRoundU8(x: f64): u8;
export fn ccHexVal(ch: u8): i16;
export fn ccAnsiStd16(idx: u8): Rgb;
export fn rgbToHsv(r: u8, g: u8, b: u8): Hsv;
export fn hsvToRgb(h: f64, s: f64, v: f64): Rgb;
export fn rgbToHsl(r: u8, g: u8, b: u8): Hsl;
export fn hslToRgb(h: f64, s: f64, l: f64): Rgb;
export fn rgbToHwb(r: u8, g: u8, b: u8): Hwb;
export fn hwbToRgb(h: f64, w: f64, b: f64): Rgb;
export fn rgbToCmyk(r: u8, g: u8, b: u8): Cmyk;
export fn cmykToRgb(c: f64, m: f64, y: f64, k: f64): Rgb;
export fn rgbToAnsi256(r: u8, g: u8, b: u8): u8;
export fn ansi256ToRgb(code: u8): Rgb;
export fn rgbToAnsi16(r: u8, g: u8, b: u8): u8;
export fn ansi256ToAnsi16(code: u8): u8;
export fn ansi16ToRgb(code: u8): Rgb;
export fn rgbToHex(r: u8, g: u8, b: u8, out: []u8): usize;
export fn hexToRgb(hex: string): RgbColor;
export fn keywordToRgb(name: string): RgbColor;
export fn rgbToKeyword(r: u8, g: u8, b: u8): string;
export fn rgbToGray(r: u8, g: u8, b: u8): f64;
export fn grayToRgb(gray: f64): Rgb;
