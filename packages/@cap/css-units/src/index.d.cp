// Type definitions for CP

export enum CssUnitKind {
    Px,
    Cm,
    Mm,
    In,
    Pt,
    Pc,
    Q,
    Rem,
    Em,
    Vh,
    Vw,
    Vmin,
    Vmax,
    Percent,
    Ch,
    Deg,
    Rad,
    Grad,
    Turn,
    Ms,
    S,
    Dpi,
    Dpcm,
    Dppx,
    Unitless,
    Unknown
}

export class CssDimension {
    valid: bool,
    value: f64,
    unit: CssUnitKind,
    unit_str: string,
}

export class CssContext {
    root_font_px: f64,
    parent_font_px: f64,
    viewport_w: f64,
    viewport_h: f64,
    fn defaultCtx(): CssContext;
}

export fn invalidDim(): CssDimension;
export fn isSpace(c: u8): bool;
export fn isDigit(c: u8): bool;
export fn isAlpha(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn strEqIgnoreCase(a: string, b: string): bool;
export fn parseUnit(u: string): CssUnitKind;
export fn isValidUnitToken(u: string): bool;
export fn parseCssDimension(raw: string): CssDimension;
export fn toPixels(dim: CssDimension, ctx: CssContext): f64?;
export fn toDegrees(dim: CssDimension): f64?;
export fn toMilliseconds(dim: CssDimension): f64?;
export fn unitCategory(u: CssUnitKind): string;
