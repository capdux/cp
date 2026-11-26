// Type definitions for CP

export class CssRgba {
    r: u8,
    g: u8,
    b: u8,
    a: f64,
    valid: bool,
}

export class CssHsla {
    h: f64,
    s: f64,
    l: f64,
    a: f64,
    valid: bool,
}

export fn ccToLower(c: u8): u8;
export fn ccIsSpace(c: u8): bool;
export fn ccTrim(s: &[]u8): &[]u8;
export fn ccEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn ccAbsF64(x: f64): f64;
export fn ccClampF64(x: f64, lo: f64, hi: f64): f64;
export fn ccClampU8(v: f64): u8;
export fn ccNormHue(h: f64): f64;
export fn hslToRgb(h: f64, s: f64, l: f64, a: f64): CssRgba;
export fn rgbToHsl(r: u8, g: u8, b: u8, a: f64): CssHsla;
export fn ccParseNum(raw: &[]u8, out_val: &mut f64, is_pct: &mut bool, allow_deg: bool): bool;
export fn ccSplitArgs(body: &[]u8, out_toks: []&[]u8): usize;
export fn parseCssColor(input: string): CssRgba;
export fn ccWriteRaw(out: []u8, pos: &mut usize, src: &[]u8): bool;
export fn ccWriteU8(out: []u8, pos: &mut usize, val: u8): bool;
export fn ccWriteAlpha(out: []u8, pos: &mut usize, a: f64): bool;
export fn formatRgb(c: CssRgba, out: []u8): usize;
