// Type definitions for CP

export class Rgb {
    r: f64,
    g: f64,
    b: f64,
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

export class Cmyk {
    c: f64,
    m: f64,
    y: f64,
    k: f64,
}

export fn chAbs(x: f64): f64;
export fn chClamp01(x: f64): f64;
export fn chNormHue(h: f64): f64;
export fn chSectorRgb(hn: f64, chroma: f64, m: f64): Rgb;
export fn rgbFromU8(r: u8, g: u8, b: u8): Rgb;
export fn rgbToHsl(c: Rgb): Hsl;
export fn hslToRgb(c: Hsl): Rgb;
export fn rgbToHsv(c: Rgb): Hsv;
export fn hsvToRgb(c: Hsv): Rgb;
export fn rgbToCmyk(c: Rgb): Cmyk;
export fn cmykToRgb(c: Cmyk): Rgb;
export fn rotateHue(c: Hsl, degrees: f64): Hsl;
export fn lighten(c: Hsl, amount: f64): Hsl;
export fn darken(c: Hsl, amount: f64): Hsl;
export fn mixRgb(a: Rgb, b: Rgb, t: f64): Rgb;
