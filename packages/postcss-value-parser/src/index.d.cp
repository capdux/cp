// Type definitions for CP

import { isDigit, isAlpha, eqIgnoreCase, trim } from "cap:ascii";
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
export fn parseUnit(u: string): CssUnitKind;
export fn isValidUnitToken(u: string): bool;
export fn parseCssDimension(raw: string): CssDimension;
export fn toPixels(dim: CssDimension, ctx: CssContext): f64?;
export fn toDegrees(dim: CssDimension): f64?;
export fn toMilliseconds(dim: CssDimension): f64?;
export fn unitCategory(u: CssUnitKind): string;
