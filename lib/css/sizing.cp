// Source: Absorbed from /data/dev/bun/src/css/properties/size.cp and margin_padding.cp
// Pure-CP CSS Sizing & Box-Model Insets: BoxSizing, Size, MaxSize, AspectRatio,
// BoxInsets (margin/padding/inset 1-4 shorthand), LogicalInsets (block/inline 1-2 shorthand).

pub enum BoxSizing: u8 {
    content_box = 0,
    border_box = 1,

    pub static fn parse(input: []const u8): BoxSizing? {
        const s = trimAscii(input);
        if (eqIgnoreCase(s, "content-box")) return .content_box;
        if (eqIgnoreCase(s, "border-box")) return .border_box;
        return null;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .content_box => "content-box",
            .border_box => "border-box",
        };
    }
}

pub enum SizeVendorPrefix: u8 {
    none = 0,
    webkit = 1,
    moz = 2,

    pub fn asStr(): []const u8 {
        return switch (_) {
            .none => "",
            .webkit => "-webkit-",
            .moz => "-moz-",
        };
    }
}

pub enum SizeUnit: u8 {
    px = 0,
    percent = 1,
    em = 2,
    rem = 3,
    vw = 4,
    vh = 5,
    vmin = 6,
    vmax = 7,
    ch = 8,

    pub fn asStr(): []const u8 {
        return switch (_) {
            .px => "px",
            .percent => "%",
            .em => "em",
            .rem => "rem",
            .vw => "vw",
            .vh => "vh",
            .vmin => "vmin",
            .vmax => "vmax",
            .ch => "ch",
        };
    }
}

/// CSS `<length-percentage>` value.
pub struct SizeLength {
    value: f32,
    unit: SizeUnit,

    pub static fn zero(): SizeLength {
        return SizeLength{ value: 0.0, unit: .px };
    }

    pub static fn of(v: f32, u: SizeUnit): SizeLength {
        return SizeLength{ value: v, unit: u };
    }

    pub fn eql(other: SizeLength): bool {
        if (_.value == 0.0 && other.value == 0.0 && _.unit != .percent && other.unit != .percent) return true;
        return _.value == other.value && _.unit == other.unit;
    }

    pub static fn parse(input: []const u8): SizeLength? {
        return parseLength(input, true);
    }

    pub static fn parseNonNegative(input: []const u8): SizeLength? {
        return parseLength(input, false);
    }

    pub fn toCss(out: []u8): []const u8 {
        const len = writeSizeLength(out, 0, _);
        return out[0..len];
    }
}

pub enum SizeKind: u8 {
    auto = 0,
    length_percentage = 1,
    min_content = 2,
    max_content = 3,
    fit_content = 4,
    fit_content_fn = 5,
    stretch = 6,
    contain = 7,
}

/// CSS `width` / `height` / `inline-size` / `block-size` value.
pub struct Size {
    kind: SizeKind,
    prefix: SizeVendorPrefix,
    length: SizeLength,

    pub static fn autoVal(): Size {
        return Size{ kind: .auto, prefix: .none, length: SizeLength.zero() };
    }

    pub static fn lengthPct(lp: SizeLength): Size {
        return Size{ kind: .length_percentage, prefix: .none, length: lp };
    }

    pub fn eql(other: Size): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == .length_percentage || _.kind == .fit_content_fn) {
            return _.length.eql(other.length);
        }
        return _.prefix == other.prefix;
    }

    pub static fn parse(input: []const u8): Size? {
        const s = trimAscii(input);
        if (s.len == 0) return null;

        if (eqIgnoreCase(s, "auto")) return Size.autoVal();
        if (eqIgnoreCase(s, "contain")) return Size{ kind: .contain, prefix: .none, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "min-content")) return Size{ kind: .min_content, prefix: .none, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "-webkit-min-content")) return Size{ kind: .min_content, prefix: .webkit, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "-moz-min-content")) return Size{ kind: .min_content, prefix: .moz, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "max-content")) return Size{ kind: .max_content, prefix: .none, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "-webkit-max-content")) return Size{ kind: .max_content, prefix: .webkit, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "-moz-max-content")) return Size{ kind: .max_content, prefix: .moz, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "fit-content")) return Size{ kind: .fit_content, prefix: .none, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "-webkit-fit-content")) return Size{ kind: .fit_content, prefix: .webkit, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "-moz-fit-content")) return Size{ kind: .fit_content, prefix: .moz, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "stretch")) return Size{ kind: .stretch, prefix: .none, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "-webkit-fill-available")) return Size{ kind: .stretch, prefix: .webkit, length: SizeLength.zero() };
        if (eqIgnoreCase(s, "-moz-available")) return Size{ kind: .stretch, prefix: .moz, length: SizeLength.zero() };

        if (inner := stripFuncCall(s, "fit-content")) {
            const lp = SizeLength.parseNonNegative(inner) ?? return null;
            return Size{ kind: .fit_content_fn, prefix: .none, length: lp };
        }

        const lp = SizeLength.parseNonNegative(s) ?? return null;
        return Size.lengthPct(lp);
    }

    pub fn toCss(out: []u8): []const u8 {
        const len = writeSize(out, 0, _);
        return out[0..len];
    }
}

pub enum MaxSizeKind: u8 {
    none = 0,
    length_percentage = 1,
    min_content = 2,
    max_content = 3,
    fit_content = 4,
    fit_content_fn = 5,
    stretch = 6,
    contain = 7,
}

/// CSS `max-width` / `max-height` / `min-width` / `min-height` value.
pub struct MaxSize {
    kind: MaxSizeKind,
    prefix: SizeVendorPrefix,
    length: SizeLength,

    pub static fn noneVal(): MaxSize {
        return MaxSize{ kind: .none, prefix: .none, length: SizeLength.zero() };
    }

    pub static fn parse(input: []const u8): MaxSize? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        if (eqIgnoreCase(s, "none")) return MaxSize.noneVal();
        if (eqIgnoreCase(s, "auto")) return null;
        const sz = Size.parse(s) ?? return null;
        const k: MaxSizeKind = switch (sz.kind) {
            .auto => return null,
            .length_percentage => .length_percentage,
            .min_content => .min_content,
            .max_content => .max_content,
            .fit_content => .fit_content,
            .fit_content_fn => .fit_content_fn,
            .stretch => .stretch,
            .contain => .contain,
        };
        return MaxSize{ kind: k, prefix: sz.prefix, length: sz.length };
    }

    pub fn toCss(out: []u8): []const u8 {
        if (_.kind == .none) {
            const len = appendStr(out, 0, "none");
            return out[0..len];
        }
        const sz_kind: SizeKind = switch (_.kind) {
            .none => .auto,
            .length_percentage => .length_percentage,
            .min_content => .min_content,
            .max_content => .max_content,
            .fit_content => .fit_content,
            .fit_content_fn => .fit_content_fn,
            .stretch => .stretch,
            .contain => .contain,
        };
        const sz = Size{ kind: sz_kind, prefix: _.prefix, length: _.length };
        const len = writeSize(out, 0, sz);
        return out[0..len];
    }
}

/// CSS `aspect-ratio`: `auto | <ratio> | auto && <ratio>`.
pub struct AspectRatio {
    has_auto: bool,
    has_ratio: bool,
    numerator: f32,
    denominator: f32,

    pub static fn parse(input: []const u8): AspectRatio? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        if (eqIgnoreCase(s, "auto")) {
            return AspectRatio{ has_auto: true, has_ratio: false, numerator: 0.0, denominator: 1.0 };
        }

        // Tokenize by whitespace and '/'
        let has_auto = false;
        let has_num = false;
        let has_slash = false;
        let has_den = false;
        let num: f32 = 0.0;
        let den: f32 = 1.0;

        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            if (s[pos] == '/') {
                if (!has_num || has_slash || has_den) return null;
                has_slash = true;
                pos += 1;
                continue;
            }
            const start = pos;
            while (pos < s.len && !isWs(s[pos]) && s[pos] != '/') : (pos += 1) {}
            const tok = s[start..pos];

            if (eqIgnoreCase(tok, "auto")) {
                if (has_auto || (has_slash && !has_den)) return null;
                has_auto = true;
                continue;
            }
            const v = parseSignedFloat(tok) ?? return null;
            if (v < 0.0) return null;
            if (!has_num) {
                num = v;
                has_num = true;
            } else if (has_slash && !has_den) {
                if (v == 0.0) return null;
                den = v;
                has_den = true;
            } else {
                return null;
            }
        }

        if (has_slash && !has_den) return null;
        if (!has_auto && !has_num) return null;
        return AspectRatio{
            has_auto: has_auto,
            has_ratio: has_num,
            numerator: num,
            denominator: den,
        };
    }

    pub fn toCss(out: []u8): []const u8 {
        let len: usize = 0;
        if (_.has_auto) {
            len = appendStr(out, len, "auto");
        }
        if (_.has_ratio) {
            if (_.has_auto) len = appendStr(out, len, " ");
            len = writeFloat(out, len, _.numerator);
            if (_.denominator != 1.0) {
                len = appendStr(out, len, " / ");
                len = writeFloat(out, len, _.denominator);
            }
        }
        return out[0..len];
    }
}

/// `<length-percentage> | auto` used by `margin`, `padding`, `inset`.
pub struct InsetValue {
    is_auto: bool,
    length: SizeLength,

    pub static fn autoVal(): InsetValue {
        return InsetValue{ is_auto: true, length: SizeLength.zero() };
    }

    pub static fn lengthPct(lp: SizeLength): InsetValue {
        return InsetValue{ is_auto: false, length: lp };
    }

    pub fn eql(other: InsetValue): bool {
        if (_.is_auto != other.is_auto) return false;
        if (_.is_auto) return true;
        return _.length.eql(other.length);
    }

    pub static fn parse(input: []const u8): InsetValue? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        if (eqIgnoreCase(s, "auto")) return InsetValue.autoVal();
        const lp = SizeLength.parse(s) ?? return null;
        return InsetValue.lengthPct(lp);
    }

    pub fn toCss(out: []u8): []const u8 {
        const len = writeInsetValue(out, 0, _);
        return out[0..len];
    }
}

/// 4-side CSS shorthand (`margin`, `padding`, `inset`, `scroll-margin`, `scroll-padding`).
pub struct BoxInsets {
    top: InsetValue,
    right: InsetValue,
    bottom: InsetValue,
    left: InsetValue,

    pub fn eql(other: BoxInsets): bool {
        return _.top.eql(other.top) && _.right.eql(other.right) && _.bottom.eql(other.bottom) && _.left.eql(other.left);
    }

    pub static fn parse(input: []const u8): BoxInsets? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        let vals: [4]InsetValue = [InsetValue.autoVal(); 4];
        let count: usize = 0;
        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const start = pos;
            while (pos < s.len && !isWs(s[pos])) : (pos += 1) {}
            if (count >= 4) return null;
            vals[count] = InsetValue.parse(s[start..pos]) ?? return null;
            count += 1;
        }
        if (count == 0) return null;
        if (count == 1) return BoxInsets{ top: vals[0], right: vals[0], bottom: vals[0], left: vals[0] };
        if (count == 2) return BoxInsets{ top: vals[0], right: vals[1], bottom: vals[0], left: vals[1] };
        if (count == 3) return BoxInsets{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[1] };
        return BoxInsets{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[3] };
    }

    pub fn toCss(out: []u8): []const u8 {
        let len = writeInsetValue(out, 0, _.top);
        if (_.top.eql(_.right) && _.top.eql(_.bottom) && _.top.eql(_.left)) return out[0..len];

        len = appendStr(out, len, " ");
        len = writeInsetValue(out, len, _.right);
        if (_.top.eql(_.bottom) && _.right.eql(_.left)) return out[0..len];

        len = appendStr(out, len, " ");
        len = writeInsetValue(out, len, _.bottom);
        if (_.right.eql(_.left)) return out[0..len];

        len = appendStr(out, len, " ");
        len = writeInsetValue(out, len, _.left);
        return out[0..len];
    }
}

/// 2-side logical CSS shorthand (`margin-inline`, `margin-block`, `padding-inline`, `inset-block`, etc.).
pub struct LogicalInsets {
    start: InsetValue,
    end: InsetValue,

    pub static fn parse(input: []const u8): LogicalInsets? {
        const s = trimAscii(input);
        if (s.len == 0) return null;
        let vals: [2]InsetValue = [InsetValue.autoVal(); 2];
        let count: usize = 0;
        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const start = pos;
            while (pos < s.len && !isWs(s[pos])) : (pos += 1) {}
            if (count >= 2) return null;
            vals[count] = InsetValue.parse(s[start..pos]) ?? return null;
            count += 1;
        }
        if (count == 0) return null;
        if (count == 1) return LogicalInsets{ start: vals[0], end: vals[0] };
        return LogicalInsets{ start: vals[0], end: vals[1] };
    }

    pub fn toCss(out: []u8): []const u8 {
        let len = writeInsetValue(out, 0, _.start);
        if (!_.start.eql(_.end)) {
            len = appendStr(out, len, " ");
            len = writeInsetValue(out, len, _.end);
        }
        return out[0..len];
    }
}

// ---------- Internal helpers ----------

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

fn stripFuncCall(s: []const u8, name: []const u8): ([]const u8)? {
    if (s.len < name.len + 2) return null;
    if (!eqIgnoreCase(s[0..name.len], name)) return null;
    if (s[name.len] != '(' || s[s.len - 1] != ')') return null;
    return trimAscii(s[name.len + 1 .. s.len - 1]);
}

fn parseSizeUnit(u: []const u8): SizeUnit? {
    if (eqIgnoreCase(u, "px")) return .px;
    if (u.len == 1 && u[0] == '%') return .percent;
    if (eqIgnoreCase(u, "em")) return .em;
    if (eqIgnoreCase(u, "rem")) return .rem;
    if (eqIgnoreCase(u, "vw")) return .vw;
    if (eqIgnoreCase(u, "vh")) return .vh;
    if (eqIgnoreCase(u, "vmin")) return .vmin;
    if (eqIgnoreCase(u, "vmax")) return .vmax;
    if (eqIgnoreCase(u, "ch")) return .ch;
    return null;
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

fn parseLength(input: []const u8, allow_neg: bool): SizeLength? {
    const s = trimAscii(input);
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
    const num = parseSignedFloat(s[0..idx]) ?? return null;
    const unit_str = s[idx..];
    if (unit_str.len == 0) {
        if (num == 0.0) return SizeLength.zero();
        return null;
    }
    const u = parseSizeUnit(unit_str) ?? return null;
    return SizeLength.of(num, u);
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

fn writeSizeLength(out: []u8, pos: usize, lp: SizeLength): usize {
    if (lp.value == 0.0 && lp.unit != .percent) return appendStr(out, pos, "0");
    let p = writeFloat(out, pos, lp.value);
    return appendStr(out, p, lp.unit.asStr());
}

fn writeInsetValue(out: []u8, pos: usize, iv: InsetValue): usize {
    if (iv.is_auto) return appendStr(out, pos, "auto");
    return writeSizeLength(out, pos, iv.length);
}

fn writeSize(out: []u8, pos: usize, sz: Size): usize {
    switch (sz.kind) {
        .auto => return appendStr(out, pos, "auto"),
        .contain => return appendStr(out, pos, "contain"),
        .length_percentage => return writeSizeLength(out, pos, sz.length),
        .min_content => {
            let p = appendStr(out, pos, sz.prefix.asStr());
            return appendStr(out, p, "min-content");
        },
        .max_content => {
            let p = appendStr(out, pos, sz.prefix.asStr());
            return appendStr(out, p, "max-content");
        },
        .fit_content => {
            let p = appendStr(out, pos, sz.prefix.asStr());
            return appendStr(out, p, "fit-content");
        },
        .fit_content_fn => {
            let p = appendStr(out, pos, "fit-content(");
            p = writeSizeLength(out, p, sz.length);
            return appendStr(out, p, ")");
        },
        .stretch => {
            return switch (sz.prefix) {
                .none => appendStr(out, pos, "stretch"),
                .webkit => appendStr(out, pos, "-webkit-fill-available"),
                .moz => appendStr(out, pos, "-moz-available"),
            };
        },
    }
}
