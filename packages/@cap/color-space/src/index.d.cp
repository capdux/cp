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

export class Oklab {
    l: f64,
    a: f64,
    b: f64,
}

export class Oklch {
    l: f64,
    c: f64,
    h: f64,
}

export fn clamp01(x: f64): f64;
export fn absF64(x: f64): f64;
export fn sqrtF64(x: f64): f64;
export fn cbrtF64(x_in: f64): f64;
export fn lnF64(x_in: f64): f64;
export fn expF64(x_in: f64): f64;
export fn powF64(base: f64, exponent: f64): f64;
export fn sinF64(x_in: f64): f64;
export fn cosF64(x: f64): f64;
export fn atanF64(x_in: f64): f64;
export fn atan2F64(y: f64, x: f64): f64;
export fn normHue(h_in: f64): f64;
export fn srgbToLinear(c_in: f64): f64;
export fn linearToSrgb(c_in: f64): f64;
export fn rgbToHsl(c: Rgb): Hsl;
export fn hslToRgb(c: Hsl): Rgb;
export fn rgbToHsv(c: Rgb): Hsv;
export fn hsvToRgb(c: Hsv): Rgb;
export fn rgbToOklab(c: Rgb): Oklab;
export fn oklabToRgb(c: Oklab): Rgb;
export fn oklabToOklch(c: Oklab): Oklch;
export fn oklchToOklab(c: Oklch): Oklab;
export fn oklabDistance(a: Rgb, b: Rgb): f64;
export fn mixOklab(a: Rgb, b: Rgb, t_in: f64): Rgb;
