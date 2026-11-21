// Source: Absorbed from /data/dev/bun/src/css/properties/effects.cp
// Pure-CP zero-allocation CSS Filter Effects (`filter` / `backdrop-filter`)
// parser and canonical serializer.

pub enum FilterKind: u8 {
    blur = 0,
    brightness = 1,
    contrast = 2,
    grayscale = 3,
    hue_rotate = 4,
    invert = 5,
    opacity = 6,
    saturate = 7,
    sepia = 8,
    drop_shadow = 9,
    url = 10,
}

pub enum FilterLengthUnit: u8 {
    px = 0,
    em = 1,
    rem = 2,

    pub fn asStr(): []const u8 {
        return switch (_) {
            .px => "px",
            .em => "em",
            .rem => "rem",
        };
    }
}

pub enum FilterAngleUnit: u8 {
    deg = 0,
    rad = 1,
    grad = 2,
    turn = 3,

    pub fn asStr(): []const u8 {
        return switch (_) {
            .deg => "deg",
            .rad => "rad",
            .grad => "grad",
            .turn => "turn",
        };
    }
}

pub struct DropShadow {
    x_offset: f32,
    x_unit: FilterLengthUnit,
    y_offset: f32,
    y_unit: FilterLengthUnit,
    has_blur: bool,
    blur: f32,
    blur_unit: FilterLengthUnit,
    color: []const u8,
}

pub struct Filter {
    kind: FilterKind,
    /// Numeric value for blur / amount (normalized 0..N; percentages stored as 0..N, e.g. 50% -> 0.5) / angle.
    value: f32,
    is_percent: bool,
    len_unit: FilterLengthUnit,
    angle_unit: FilterAngleUnit,
    url_ref: []const u8,
    shadow: DropShadow,

    pub static fn parse(input: []const u8): Filter? {
        const s = trimAscii(input);
        if (s.len < 3 || s[s.len - 1] != ')') return null;
        let open_p: usize = 0;
        while (open_p < s.len && s[open_p] != '(') : (open_p += 1) {}
        if (open_p == 0 || open_p >= s.len - 1) return null;

        const fname = trimAscii(s[0..open_p]);
        const inner = trimAscii(s[open_p + 1 .. s.len - 1]);

        if (eqIgnoreCase(fname, "blur")) {
            if (inner.len == 0) return makeBlur(0.0, .px);
            const len_v = parseLength(inner, false) ?? return null;
            return makeBlur(len_v.val, len_v.unit);
        }
        if (eqIgnoreCase(fname, "brightness")) return parseAmountFilter(.brightness, inner, 1.0, false);
        if (eqIgnoreCase(fname, "contrast")) return parseAmountFilter(.contrast, inner, 1.0, false);
        if (eqIgnoreCase(fname, "grayscale")) return parseAmountFilter(.grayscale, inner, 1.0, true);
        if (eqIgnoreCase(fname, "invert")) return parseAmountFilter(.invert, inner, 1.0, true);
        if (eqIgnoreCase(fname, "opacity")) return parseAmountFilter(.opacity, inner, 1.0, true);
        if (eqIgnoreCase(fname, "saturate")) return parseAmountFilter(.saturate, inner, 1.0, false);
        if (eqIgnoreCase(fname, "sepia")) return parseAmountFilter(.sepia, inner, 1.0, true);

        if (eqIgnoreCase(fname, "hue-rotate")) {
            if (inner.len == 0) return makeHueRotate(0.0, .deg);
            const ang = parseAngle(inner) ?? return null;
            return makeHueRotate(ang.val, ang.unit);
        }

        if (eqIgnoreCase(fname, "url")) {
            const unq = stripQuotes(inner);
            if (unq.len == 0) return null;
            return makeUrl(unq);
        }

        if (eqIgnoreCase(fname, "drop-shadow")) {
            const ds = parseDropShadow(inner) ?? return null;
            return makeDropShadow(ds);
        }

        return null;
    }

    pub fn toCss(out: []u8): []const u8 {
        const len = writeFilter(out, 0, _);
        return out[0..len];
    }
}

/// CSS `filter` / `backdrop-filter` value: `none` or up to 16 chained filter functions.
pub struct FilterList {
    is_none: bool,
    items: [16]Filter,
    len: usize,

    pub static fn noneVal(): FilterList {
        return FilterList{
            is_none: true,
            items: [makeBlur(0.0, .px); 16],
            len: 0,
        };
    }

    pub static fn parse(input: []const u8): FilterList? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        if (eqIgnoreCase(s, "none")) return FilterList.noneVal();

        let res = FilterList{
            is_none: false,
            items: [makeBlur(0.0, .px); 16],
            len: 0,
        };

        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;

            const start = pos;
            let depth: u32 = 0;
            let saw_paren = false;
            while (pos < s.len) : (pos += 1) {
                if (s[pos] == '(') {
                    depth += 1;
                    saw_paren = true;
                } else if (s[pos] == ')') {
                    if (depth == 0) return null;
                    depth -= 1;
                    if (depth == 0) {
                        pos += 1;
                        break;
                    }
                } else if (isWs(s[pos]) && depth == 0) {
                    break;
                }
            }
            if (!saw_paren || depth != 0) return null;
            if (res.len >= 16) return null;

            const f = Filter.parse(s[start..pos]) ?? return null;
            res.items[res.len] = f;
            res.len += 1;
        }

        if (res.len == 0) return null;
        return res;
    }

    pub fn toCss(out: []u8): []const u8 {
        if (_.is_none || _.len == 0) {
            const l = appendStr(out, 0, "none");
            return out[0..l];
        }
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (i > 0) pos = appendStr(out, pos, " ");
            pos = writeFilter(out, pos, _.items[i]);
        }
        return out[0..pos];
    }
}

// ---------- Constructors & Parsers ----------

struct ParsedLen { val: f32, unit: FilterLengthUnit }
struct ParsedAngle { val: f32, unit: FilterAngleUnit }

fn emptyShadow(): DropShadow {
    return DropShadow{
        x_offset: 0.0,
        x_unit: .px,
        y_offset: 0.0,
        y_unit: .px,
        has_blur: false,
        blur: 0.0,
        blur_unit: .px,
        color: "",
    };
}

fn makeBlur(val: f32, unit: FilterLengthUnit): Filter {
    return Filter{
        kind: .blur,
        value: val,
        is_percent: false,
        len_unit: unit,
        angle_unit: .deg,
        url_ref: "",
        shadow: emptyShadow(),
    };
}

fn makeHueRotate(val: f32, unit: FilterAngleUnit): Filter {
    return Filter{
        kind: .hue_rotate,
        value: val,
        is_percent: false,
        len_unit: .px,
        angle_unit: unit,
        url_ref: "",
        shadow: emptyShadow(),
    };
}

fn makeUrl(u: []const u8): Filter {
    return Filter{
        kind: .url,
        value: 0.0,
        is_percent: false,
        len_unit: .px,
        angle_unit: .deg,
        url_ref: u,
        shadow: emptyShadow(),
    };
}

fn makeDropShadow(ds: DropShadow): Filter {
    return Filter{
        kind: .drop_shadow,
        value: 0.0,
        is_percent: false,
        len_unit: .px,
        angle_unit: .deg,
        url_ref: "",
        shadow: ds,
    };
}

fn parseAmountFilter(kind: FilterKind, inner: []const u8, default_val: f32, clamp_unit: bool): Filter? {
    if (inner.len == 0) {
        return Filter{
            kind: kind,
            value: default_val,
            is_percent: false,
            len_unit: .px,
            angle_unit: .deg,
            url_ref: "",
            shadow: emptyShadow(),
        };
    }
    let is_pct = false;
    let num_str = inner;
    if (inner[inner.len - 1] == '%') {
        is_pct = true;
        num_str = inner[0 .. inner.len - 1];
    }
    const raw = parseSignedFloat(num_str) ?? return null;
    if (raw < 0.0) return null;
    let norm = if (is_pct) raw / 100.0 else raw;
    if (clamp_unit && norm > 1.0) norm = 1.0;
    return Filter{
        kind: kind,
        value: norm,
        is_percent: is_pct,
        len_unit: .px,
        angle_unit: .deg,
        url_ref: "",
        shadow: emptyShadow(),
    };
}

fn parseDropShadow(inner: []const u8): DropShadow? {
    if (inner.len == 0) return null;
    let lengths: [3]ParsedLen = [ParsedLen{ val: 0.0, unit: .px }; 3];
    let len_count: usize = 0;
    let color: []const u8 = "";

    let pos: usize = 0;
    while (pos < inner.len) {
        while (pos < inner.len && isWs(inner[pos])) : (pos += 1) {}
        if (pos >= inner.len) break;
        const start = pos;
        let depth: u32 = 0;
        while (pos < inner.len) : (pos += 1) {
            if (inner[pos] == '(') {
                depth += 1;
            } else if (inner[pos] == ')') {
                if (depth == 0) return null;
                depth -= 1;
            } else if (isWs(inner[pos]) && depth == 0) {
                break;
            }
        }
        if (depth != 0) return null;
        const tok = inner[start..pos];

        if (lv := parseLength(tok, true)) {
            if (len_count >= 3) return null;
            if (len_count == 2 && lv.val < 0.0) return null; // negative blur radius invalid
            lengths[len_count] = lv;
            len_count += 1;
        } else {
            if (color.len > 0) return null;
            color = tok;
        }
    }

    if (len_count < 2) return null;
    return DropShadow{
        x_offset: lengths[0].val,
        x_unit: lengths[0].unit,
        y_offset: lengths[1].val,
        y_unit: lengths[1].unit,
        has_blur: len_count == 3,
        blur: if (len_count == 3) lengths[2].val else 0.0,
        blur_unit: if (len_count == 3) lengths[2].unit else .px,
        color: color,
    };
}

fn parseLength(s_in: []const u8, allow_neg: bool): ParsedLen? {
    const s = trimAscii(s_in);
    if (s.len == 0) return null;
    let idx: usize = 0;
    if (s[0] == '-' || s[0] == '+') {
        if (s[0] == '-' && !allow_neg) return null;
        idx = 1;
    }
    let has_digit = false;
    let has_dot = false;
    while (idx < s.len) : (idx += 1) {
        const c = s[idx];
        if (c >= '0' && c <= '9') {
            has_digit = true;
        } else if (c == '.' && !has_dot) {
            has_dot = true;
        } else {
            break;
        }
    }
    if (!has_digit) return null;
    const val = parseSignedFloat(s[0..idx]) ?? return null;
    const u = s[idx..];
    if (u.len == 0) {
        if (val == 0.0) return ParsedLen{ val: 0.0, unit: .px };
        return null;
    }
    if (eqIgnoreCase(u, "px")) return ParsedLen{ val: val, unit: .px };
    if (eqIgnoreCase(u, "em")) return ParsedLen{ val: val, unit: .em };
    if (eqIgnoreCase(u, "rem")) return ParsedLen{ val: val, unit: .rem };
    return null;
}

fn parseAngle(s_in: []const u8): ParsedAngle? {
    const s = trimAscii(s_in);
    if (s.len == 0) return null;
    let idx: usize = 0;
    if (s[0] == '-' || s[0] == '+') idx = 1;
    let has_digit = false;
    let has_dot = false;
    while (idx < s.len) : (idx += 1) {
        const c = s[idx];
        if (c >= '0' && c <= '9') {
            has_digit = true;
        } else if (c == '.' && !has_dot) {
            has_dot = true;
        } else {
            break;
        }
    }
    if (!has_digit) return null;
    const val = parseSignedFloat(s[0..idx]) ?? return null;
    const u = s[idx..];
    if (u.len == 0) {
        if (val == 0.0) return ParsedAngle{ val: 0.0, unit: .deg };
        return null;
    }
    if (eqIgnoreCase(u, "deg")) return ParsedAngle{ val: val, unit: .deg };
    if (eqIgnoreCase(u, "rad")) return ParsedAngle{ val: val, unit: .rad };
    if (eqIgnoreCase(u, "grad")) return ParsedAngle{ val: val, unit: .grad };
    if (eqIgnoreCase(u, "turn")) return ParsedAngle{ val: val, unit: .turn };
    return null;
}

// ---------- Serializer ----------

fn writeFilter(out: []u8, pos: usize, f: Filter): usize {
    let p = pos;
    switch (f.kind) {
        .blur => {
            p = appendStr(out, p, "blur(");
            p = writeLen(out, p, f.value, f.len_unit);
            return appendStr(out, p, ")");
        },
        .brightness => return writeAmount(out, p, "brightness", f),
        .contrast => return writeAmount(out, p, "contrast", f),
        .grayscale => return writeAmount(out, p, "grayscale", f),
        .invert => return writeAmount(out, p, "invert", f),
        .opacity => return writeAmount(out, p, "opacity", f),
        .saturate => return writeAmount(out, p, "saturate", f),
        .sepia => return writeAmount(out, p, "sepia", f),
        .hue_rotate => {
            p = appendStr(out, p, "hue-rotate(");
            if (f.value == 0.0) {
                p = appendStr(out, p, "0deg");
            } else {
                p = writeFloat(out, p, f.value);
                p = appendStr(out, p, f.angle_unit.asStr());
            }
            return appendStr(out, p, ")");
        },
        .url => {
            p = appendStr(out, p, "url(");
            p = appendStr(out, p, f.url_ref);
            return appendStr(out, p, ")");
        },
        .drop_shadow => {
            p = appendStr(out, p, "drop-shadow(");
            p = writeLen(out, p, f.shadow.x_offset, f.shadow.x_unit);
            p = appendStr(out, p, " ");
            p = writeLen(out, p, f.shadow.y_offset, f.shadow.y_unit);
            if (f.shadow.has_blur && f.shadow.blur != 0.0) {
                p = appendStr(out, p, " ");
                p = writeLen(out, p, f.shadow.blur, f.shadow.blur_unit);
            }
            if (f.shadow.color.len > 0) {
                p = appendStr(out, p, " ");
                p = appendStr(out, p, f.shadow.color);
            }
            return appendStr(out, p, ")");
        },
    }
}

fn writeAmount(out: []u8, pos: usize, name: []const u8, f: Filter): usize {
    let p = appendStr(out, pos, name);
    p = appendStr(out, p, "(");
    if (f.is_percent) {
        p = writeFloat(out, p, f.value * 100.0);
        p = appendStr(out, p, "%");
    } else {
        p = writeFloat(out, p, f.value);
    }
    return appendStr(out, p, ")");
}

fn writeLen(out: []u8, pos: usize, val: f32, unit: FilterLengthUnit): usize {
    if (val == 0.0) return appendStr(out, pos, "0");
    const p = writeFloat(out, pos, val);
    return appendStr(out, p, unit.asStr());
}

// ---------- Low-level helpers ----------

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) : (start += 1) {}
    let end = s.len;
    while (end > start && isWs(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn stripQuotes(s: []const u8): []const u8 {
    if (s.len >= 2 && ((s[0] == '"' && s[s.len - 1] == '"') || (s[0] == '\'' && s[s.len - 1] == '\''))) {
        return s[1 .. s.len - 1];
    }
    return s;
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}

fn parseSignedFloat(s: []const u8): f32? {
    if (s.len == 0) return null;
    let idx: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        idx = 1;
    } else if (s[0] == '+') {
        idx = 1;
    }
    let int_part: f64 = 0.0;
    let frac_part: f64 = 0.0;
    let frac_div: f64 = 1.0;
    let in_frac = false;
    let digits: usize = 0;
    while (idx < s.len) : (idx += 1) {
        const c = s[idx];
        if (c >= '0' && c <= '9') {
            const d: f64 = @floatFromInt(c - '0');
            if (in_frac) {
                frac_div *= 10.0;
                frac_part += d / frac_div;
            } else {
                int_part = int_part * 10.0 + d;
            }
            digits += 1;
        } else if (c == '.' && !in_frac) {
            in_frac = true;
        } else {
            return null;
        }
    }
    if (digits == 0) return null;
    const val = int_part + frac_part;
    return @floatCast(if (neg) 0.0 - val else val);
}

fn appendStr(out: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn appendU32(out: []u8, pos: usize, v_in: u32): usize {
    if (v_in == 0) return appendStr(out, pos, "0");
    let buf: [16]u8 = [0; 16];
    let n: usize = 0;
    let v = v_in;
    while (v > 0) : (v /= 10) {
        buf[n] = @intCast('0' + (v % 10));
        n += 1;
    }
    let p = pos;
    while (n > 0 && p < out.len) {
        n -= 1;
        out[p] = buf[n];
        p += 1;
    }
    return p;
}

fn writeFloat(out: []u8, pos: usize, v_in: f32): usize {
    let p = pos;
    let v = v_in;
    if (v < 0.0) {
        p = appendStr(out, p, "-");
        v = 0.0 - v;
    }
    const scaled: u32 = @intFromFloat(v * 100.0 + 0.5);
    const whole = scaled / 100;
    const frac = scaled % 100;
    p = appendU32(out, p, whole);
    if (frac != 0) {
        p = appendStr(out, p, ".");
        if (frac % 10 == 0) {
            p = appendU32(out, p, frac / 10);
        } else {
            if (frac < 10) p = appendStr(out, p, "0");
            p = appendU32(out, p, frac);
        }
    }
    return p;
}
