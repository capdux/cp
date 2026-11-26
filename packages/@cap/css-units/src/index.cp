// @cap/css-units — Zero-allocation CSS dimension/length/angle/time/resolution parser & converter for CP
// Clean-room MIT implementation inspired by `postcss-value-parser` (120M/wk), `parse-css-dimension`, and `css-unit-converter`.

const PI: f64 = 3.14159265358979323846;

pub enum CssUnitKind {
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
    Unknown,
}

pub struct CssDimension {
    pub valid: bool,
    pub value: f64,
    pub unit: CssUnitKind,
    pub unit_str: string,
}

pub struct CssContext {
    pub root_font_px: f64,
    pub parent_font_px: f64,
    pub viewport_w: f64,
    pub viewport_h: f64,

    pub static fn defaultCtx() -> CssContext {
        return CssContext{
            root_font_px: 16.0,
            parent_font_px: 16.0,
            viewport_w: 1920.0,
            viewport_h: 1080.0,
        };
    }
}

fn invalidDim() -> CssDimension {
    return CssDimension{
        valid: false,
        value: 0.0,
        unit: CssUnitKind.Unknown,
        unit_str: "",
    };
}

fn isSpace(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8) -> bool {
    return c >= '0' && c <= '9';
}

fn isAlpha(c: u8) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn strEqIgnoreCase(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parseUnit(u: string) -> CssUnitKind {
    if (u.len == 0) { return CssUnitKind.Unitless; }
    if (u.len == 1 && u[0] == '%') { return CssUnitKind.Percent; }
    if (strEqIgnoreCase(u, "px")) { return CssUnitKind.Px; }
    if (strEqIgnoreCase(u, "cm")) { return CssUnitKind.Cm; }
    if (strEqIgnoreCase(u, "mm")) { return CssUnitKind.Mm; }
    if (strEqIgnoreCase(u, "in")) { return CssUnitKind.In; }
    if (strEqIgnoreCase(u, "pt")) { return CssUnitKind.Pt; }
    if (strEqIgnoreCase(u, "pc")) { return CssUnitKind.Pc; }
    if (strEqIgnoreCase(u, "q")) { return CssUnitKind.Q; }
    if (strEqIgnoreCase(u, "rem")) { return CssUnitKind.Rem; }
    if (strEqIgnoreCase(u, "em")) { return CssUnitKind.Em; }
    if (strEqIgnoreCase(u, "vh")) { return CssUnitKind.Vh; }
    if (strEqIgnoreCase(u, "vw")) { return CssUnitKind.Vw; }
    if (strEqIgnoreCase(u, "vmin")) { return CssUnitKind.Vmin; }
    if (strEqIgnoreCase(u, "vmax")) { return CssUnitKind.Vmax; }
    if (strEqIgnoreCase(u, "ch")) { return CssUnitKind.Ch; }
    if (strEqIgnoreCase(u, "deg")) { return CssUnitKind.Deg; }
    if (strEqIgnoreCase(u, "rad")) { return CssUnitKind.Rad; }
    if (strEqIgnoreCase(u, "grad")) { return CssUnitKind.Grad; }
    if (strEqIgnoreCase(u, "turn")) { return CssUnitKind.Turn; }
    if (strEqIgnoreCase(u, "ms")) { return CssUnitKind.Ms; }
    if (strEqIgnoreCase(u, "s")) { return CssUnitKind.S; }
    if (strEqIgnoreCase(u, "dpi")) { return CssUnitKind.Dpi; }
    if (strEqIgnoreCase(u, "dpcm")) { return CssUnitKind.Dpcm; }
    if (strEqIgnoreCase(u, "dppx")) { return CssUnitKind.Dppx; }
    return CssUnitKind.Unknown;
}

fn isValidUnitToken(u: string) -> bool {
    if (u.len == 0) {
        return true;
    }
    if (u.len == 1 && u[0] == '%') {
        return true;
    }
    let i: usize = 0;
    while (i < u.len) {
        const c = u[i];
        if (!isAlpha(c) && !isDigit(c) && c != '-' && c != '_') {
            return false;
        }
        i += 1;
    }
    return isAlpha(u[0]) || u[0] == '-' || u[0] == '_';
}

pub fn parseCssDimension(raw: string) -> CssDimension {
    let start: usize = 0;
    while (start < raw.len && isSpace(raw[start])) {
        start += 1;
    }
    let end: usize = raw.len;
    while (end > start && isSpace(raw[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return invalidDim();
    }

    const s = raw[start..end];
    let i: usize = 0;
    let sign: f64 = 1.0;
    if (s[i] == '+') {
        i += 1;
    } else if (s[i] == '-') {
        sign = -1.0;
        i += 1;
    }

    let val: f64 = 0.0;
    let hasDigits = false;
    while (i < s.len && isDigit(s[i])) {
        val = val * 10.0 + ((s[i] - '0') as f64);
        hasDigits = true;
        i += 1;
    }

    if (i < s.len && s[i] == '.') {
        i += 1;
        let fracScale: f64 = 0.1;
        let hasFrac = false;
        while (i < s.len && isDigit(s[i])) {
            val += ((s[i] - '0') as f64) * fracScale;
            fracScale *= 0.1;
            hasDigits = true;
            hasFrac = true;
            i += 1;
        }
        if (!hasFrac && !hasDigits) {
            return invalidDim();
        }
    }

    if (!hasDigits) {
        return invalidDim();
    }

    // Optional scientific notation: only if 'e'/'E' is followed by digits or (+/-)digits
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        let expPos = i + 1;
        let expNeg = false;
        if (expPos < s.len && (s[expPos] == '+' || s[expPos] == '-')) {
            expNeg = s[expPos] == '-';
            expPos += 1;
        }
        if (expPos < s.len && isDigit(s[expPos])) {
            let expVal: i32 = 0;
            while (expPos < s.len && isDigit(s[expPos])) {
                if (expVal < 308) {
                    expVal = expVal * 10 + ((s[expPos] - '0') as i32);
                }
                expPos += 1;
            }
            let e: i32 = 0;
            while (e < expVal) {
                if (expNeg) {
                    val /= 10.0;
                } else {
                    val *= 10.0;
                }
                e += 1;
            }
            i = expPos;
        }
    }

    const unitStr = s[i..s.len];
    if (!isValidUnitToken(unitStr)) {
        return invalidDim();
    }
    const unitKind = parseUnit(unitStr);
    return CssDimension{
        valid: true,
        value: sign * val,
        unit: unitKind,
        unit_str: unitStr,
    };
}

pub fn toPixels(dim: CssDimension, ctx: CssContext) -> f64? {
    if (!dim.valid) {
        return null;
    }
    switch (dim.unit) {
        CssUnitKind.Px => { return dim.value; },
        CssUnitKind.In => { return dim.value * 96.0; },
        CssUnitKind.Cm => { return dim.value * (96.0 / 2.54); },
        CssUnitKind.Mm => { return dim.value * (96.0 / 25.4); },
        CssUnitKind.Q => { return dim.value * (96.0 / 101.6); },
        CssUnitKind.Pt => { return dim.value * (96.0 / 72.0); },
        CssUnitKind.Pc => { return dim.value * 16.0; },
        CssUnitKind.Rem => { return dim.value * ctx.root_font_px; },
        CssUnitKind.Em => { return dim.value * ctx.parent_font_px; },
        CssUnitKind.Ch => { return dim.value * (ctx.parent_font_px * 0.5); },
        CssUnitKind.Vh => { return dim.value * (ctx.viewport_h / 100.0); },
        CssUnitKind.Vw => { return dim.value * (ctx.viewport_w / 100.0); },
        CssUnitKind.Vmin => {
            const m = if (ctx.viewport_w < ctx.viewport_h) { ctx.viewport_w } else { ctx.viewport_h };
            return dim.value * (m / 100.0);
        },
        CssUnitKind.Vmax => {
            const m = if (ctx.viewport_w > ctx.viewport_h) { ctx.viewport_w } else { ctx.viewport_h };
            return dim.value * (m / 100.0);
        },
        CssUnitKind.Unitless => {
            if (dim.value == 0.0) {
                return 0.0;
            }
            return null;
        },
        CssUnitKind.Percent => { return null; },
        CssUnitKind.Deg => { return null; },
        CssUnitKind.Rad => { return null; },
        CssUnitKind.Grad => { return null; },
        CssUnitKind.Turn => { return null; },
        CssUnitKind.Ms => { return null; },
        CssUnitKind.S => { return null; },
        CssUnitKind.Dpi => { return null; },
        CssUnitKind.Dpcm => { return null; },
        CssUnitKind.Dppx => { return null; },
        CssUnitKind.Unknown => { return null; },
    }
}

pub fn toDegrees(dim: CssDimension) -> f64? {
    if (!dim.valid) {
        return null;
    }
    switch (dim.unit) {
        CssUnitKind.Deg => { return dim.value; },
        CssUnitKind.Rad => { return dim.value * (180.0 / PI); },
        CssUnitKind.Grad => { return dim.value * 0.9; },
        CssUnitKind.Turn => { return dim.value * 360.0; },
        CssUnitKind.Px => { return null; },
        CssUnitKind.Cm => { return null; },
        CssUnitKind.Mm => { return null; },
        CssUnitKind.In => { return null; },
        CssUnitKind.Pt => { return null; },
        CssUnitKind.Pc => { return null; },
        CssUnitKind.Q => { return null; },
        CssUnitKind.Rem => { return null; },
        CssUnitKind.Em => { return null; },
        CssUnitKind.Vh => { return null; },
        CssUnitKind.Vw => { return null; },
        CssUnitKind.Vmin => { return null; },
        CssUnitKind.Vmax => { return null; },
        CssUnitKind.Percent => { return null; },
        CssUnitKind.Ch => { return null; },
        CssUnitKind.Ms => { return null; },
        CssUnitKind.S => { return null; },
        CssUnitKind.Dpi => { return null; },
        CssUnitKind.Dpcm => { return null; },
        CssUnitKind.Dppx => { return null; },
        CssUnitKind.Unitless => { return null; },
        CssUnitKind.Unknown => { return null; },
    }
}

pub fn toMilliseconds(dim: CssDimension) -> f64? {
    if (!dim.valid) {
        return null;
    }
    switch (dim.unit) {
        CssUnitKind.Ms => { return dim.value; },
        CssUnitKind.S => { return dim.value * 1000.0; },
        CssUnitKind.Px => { return null; },
        CssUnitKind.Cm => { return null; },
        CssUnitKind.Mm => { return null; },
        CssUnitKind.In => { return null; },
        CssUnitKind.Pt => { return null; },
        CssUnitKind.Pc => { return null; },
        CssUnitKind.Q => { return null; },
        CssUnitKind.Rem => { return null; },
        CssUnitKind.Em => { return null; },
        CssUnitKind.Vh => { return null; },
        CssUnitKind.Vw => { return null; },
        CssUnitKind.Vmin => { return null; },
        CssUnitKind.Vmax => { return null; },
        CssUnitKind.Percent => { return null; },
        CssUnitKind.Ch => { return null; },
        CssUnitKind.Deg => { return null; },
        CssUnitKind.Rad => { return null; },
        CssUnitKind.Grad => { return null; },
        CssUnitKind.Turn => { return null; },
        CssUnitKind.Dpi => { return null; },
        CssUnitKind.Dpcm => { return null; },
        CssUnitKind.Dppx => { return null; },
        CssUnitKind.Unitless => { return null; },
        CssUnitKind.Unknown => { return null; },
    }
}

pub fn unitCategory(u: CssUnitKind) -> string {
    switch (u) {
        CssUnitKind.Px => { return "length"; },
        CssUnitKind.Cm => { return "length"; },
        CssUnitKind.Mm => { return "length"; },
        CssUnitKind.In => { return "length"; },
        CssUnitKind.Pt => { return "length"; },
        CssUnitKind.Pc => { return "length"; },
        CssUnitKind.Q => { return "length"; },
        CssUnitKind.Rem => { return "length"; },
        CssUnitKind.Em => { return "length"; },
        CssUnitKind.Vh => { return "length"; },
        CssUnitKind.Vw => { return "length"; },
        CssUnitKind.Vmin => { return "length"; },
        CssUnitKind.Vmax => { return "length"; },
        CssUnitKind.Ch => { return "length"; },
        CssUnitKind.Percent => { return "percentage"; },
        CssUnitKind.Deg => { return "angle"; },
        CssUnitKind.Rad => { return "angle"; },
        CssUnitKind.Grad => { return "angle"; },
        CssUnitKind.Turn => { return "angle"; },
        CssUnitKind.Ms => { return "time"; },
        CssUnitKind.S => { return "time"; },
        CssUnitKind.Dpi => { return "resolution"; },
        CssUnitKind.Dpcm => { return "resolution"; },
        CssUnitKind.Dppx => { return "resolution"; },
        CssUnitKind.Unitless => { return "none"; },
        CssUnitKind.Unknown => { return "none"; },
    }
}

export {
    CssUnitKind,
    CssDimension,
    CssContext,
    parseCssDimension,
    toPixels,
    toDegrees,
    toMilliseconds,
    unitCategory,
};
