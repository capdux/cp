// Source: Absorbed from /data/dev/bun/src/css/values/position.cp
// Pure-CP zero-allocation CSS `<position>` 1/2/3/4-value parser and canonical
// serializer (used by background-position, object-position, mask-position, gradients).

pub enum HorizontalSide: u8 {
    left = 0,
    right = 1,

    pub fn asStr(): []const u8 {
        return if (_ == .left) "left" else "right";
    }

    pub fn asPercent(): f32 {
        return if (_ == .left) 0.0 else 100.0;
    }
}

pub enum VerticalSide: u8 {
    top = 0,
    bottom = 1,

    pub fn asStr(): []const u8 {
        return if (_ == .top) "top" else "bottom";
    }

    pub fn asPercent(): f32 {
        return if (_ == .top) 0.0 else 100.0;
    }
}

pub enum PosUnit: u8 {
    px = 0,
    percent = 1,
    em = 2,
    rem = 3,

    pub fn asStr(): []const u8 {
        return switch (_) {
            .px => "px",
            .percent => "%",
            .em => "em",
            .rem => "rem",
        };
    }
}

pub struct PosLength {
    value: f32,
    unit: PosUnit,

    pub static fn zero(): PosLength {
        return PosLength{ value: 0.0, unit: .px };
    }

    pub static fn pct(p: f32): PosLength {
        return PosLength{ value: p, unit: .percent };
    }

    pub fn isZero(): bool {
        return _.value == 0.0;
    }

    pub fn eql(other: PosLength): bool {
        if (_.value == 0.0 && other.value == 0.0 && _.unit != .percent && other.unit != .percent) return true;
        return _.value == other.value && _.unit == other.unit;
    }

    pub static fn parse(input: []const u8): PosLength? {
        const s = trimAscii(input);
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
            if (val == 0.0) return PosLength.zero();
            return null;
        }
        if (u.len == 1 && u[0] == '%') return PosLength.pct(val);
        if (eqIgnoreCase(u, "px")) return PosLength{ value: val, unit: .px };
        if (eqIgnoreCase(u, "em")) return PosLength{ value: val, unit: .em };
        if (eqIgnoreCase(u, "rem")) return PosLength{ value: val, unit: .rem };
        return null;
    }
}

pub enum AxisPosKind: u8 {
    center = 0,
    length = 1,
    side = 2,
}

pub struct HorizontalPosition {
    kind: AxisPosKind,
    side: HorizontalSide,
    has_offset: bool,
    length: PosLength,

    pub static fn centerVal(): HorizontalPosition {
        return HorizontalPosition{ kind: .center, side: .left, has_offset: false, length: PosLength.pct(50.0) };
    }

    pub static fn fromLength(l: PosLength): HorizontalPosition {
        return HorizontalPosition{ kind: .length, side: .left, has_offset: false, length: l };
    }

    pub static fn fromSide(s: HorizontalSide, offset: PosLength?): HorizontalPosition {
        if (off := offset) {
            return HorizontalPosition{ kind: .side, side: s, has_offset: true, length: off };
        }
        return HorizontalPosition{ kind: .side, side: s, has_offset: false, length: PosLength.zero() };
    }

    pub fn isCenter(): bool {
        if (_.kind == .center) return true;
        if (_.kind == .length && _.length.unit == .percent && _.length.value == 50.0) return true;
        return false;
    }

    pub fn eql(other: HorizontalPosition): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == .center) return true;
        if (_.kind == .length) return _.length.eql(other.length);
        return _.side == other.side && _.has_offset == other.has_offset && (!_.has_offset || _.length.eql(other.length));
    }
}

pub struct VerticalPosition {
    kind: AxisPosKind,
    side: VerticalSide,
    has_offset: bool,
    length: PosLength,

    pub static fn centerVal(): VerticalPosition {
        return VerticalPosition{ kind: .center, side: .top, has_offset: false, length: PosLength.pct(50.0) };
    }

    pub static fn fromLength(l: PosLength): VerticalPosition {
        return VerticalPosition{ kind: .length, side: .top, has_offset: false, length: l };
    }

    pub static fn fromSide(s: VerticalSide, offset: PosLength?): VerticalPosition {
        if (off := offset) {
            return VerticalPosition{ kind: .side, side: s, has_offset: true, length: off };
        }
        return VerticalPosition{ kind: .side, side: s, has_offset: false, length: PosLength.zero() };
    }

    pub fn isCenter(): bool {
        if (_.kind == .center) return true;
        if (_.kind == .length && _.length.unit == .percent && _.length.value == 50.0) return true;
        return false;
    }

    pub fn eql(other: VerticalPosition): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == .center) return true;
        if (_.kind == .length) return _.length.eql(other.length);
        return _.side == other.side && _.has_offset == other.has_offset && (!_.has_offset || _.length.eql(other.length));
    }
}

/// CSS `<position>` 2D coordinate with full 1/2/3/4-value syntax support.
pub struct CssPosition {
    x: HorizontalPosition,
    y: VerticalPosition,

    pub static fn centerVal(): CssPosition {
        return CssPosition{ x: HorizontalPosition.centerVal(), y: VerticalPosition.centerVal() };
    }

    pub fn isCenter(): bool {
        return _.x.isCenter() && _.y.isCenter();
    }

    pub static fn parse(input: []const u8): CssPosition? {
        const s = trimAscii(input);
        if (s.len == 0) return null;

        let toks: [4][]const u8 = [""; 4];
        let n: usize = 0;
        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isWs(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;
            const start = pos;
            while (pos < s.len && !isWs(s[pos])) : (pos += 1) {}
            if (n >= 4) return null;
            toks[n] = s[start..pos];
            n += 1;
        }
        if (n == 0) return null;

        if (n == 1) {
            const t = toks[0];
            if (eqIgnoreCase(t, "center")) return CssPosition.centerVal();
            if (hs := parseHSide(t)) return CssPosition{ x: HorizontalPosition.fromSide(hs, null), y: VerticalPosition.centerVal() };
            if (vs := parseVSide(t)) return CssPosition{ x: HorizontalPosition.centerVal(), y: VerticalPosition.fromSide(vs, null) };
            if (lp := PosLength.parse(t)) return CssPosition{ x: HorizontalPosition.fromLength(lp), y: VerticalPosition.centerVal() };
            return null;
        }

        if (n == 2) {
            const t0 = toks[0];
            const t1 = toks[1];
            // Case A: vertical keyword first -> only allowed if second is horizontal keyword or center
            if (vs := parseVSide(t0)) {
                if (eqIgnoreCase(t1, "center")) {
                    return CssPosition{ x: HorizontalPosition.centerVal(), y: VerticalPosition.fromSide(vs, null) };
                }
                if (hs := parseHSide(t1)) {
                    return CssPosition{ x: HorizontalPosition.fromSide(hs, null), y: VerticalPosition.fromSide(vs, null) };
                }
                return null;
            }
            // Case B: center first
            if (eqIgnoreCase(t0, "center")) {
                if (eqIgnoreCase(t1, "center")) return CssPosition.centerVal();
                if (vs := parseVSide(t1)) return CssPosition{ x: HorizontalPosition.centerVal(), y: VerticalPosition.fromSide(vs, null) };
                if (hs := parseHSide(t1)) return CssPosition{ x: HorizontalPosition.fromSide(hs, null), y: VerticalPosition.centerVal() };
                if (lp := PosLength.parse(t1)) return CssPosition{ x: HorizontalPosition.centerVal(), y: VerticalPosition.fromLength(lp) };
                return null;
            }
            // Case C: horizontal keyword or length first -> second is vertical keyword, center, or length
            let x: HorizontalPosition = HorizontalPosition.centerVal();
            if (hs := parseHSide(t0)) {
                x = HorizontalPosition.fromSide(hs, null);
            } else if (lp0 := PosLength.parse(t0)) {
                x = HorizontalPosition.fromLength(lp0);
            } else {
                return null;
            }
            if (eqIgnoreCase(t1, "center")) return CssPosition{ x: x, y: VerticalPosition.centerVal() };
            if (vs := parseVSide(t1)) return CssPosition{ x: x, y: VerticalPosition.fromSide(vs, null) };
            if (lp1 := PosLength.parse(t1)) return CssPosition{ x: x, y: VerticalPosition.fromLength(lp1) };
            return null;
        }

        if (n == 3) {
            // Valid 3-value forms:
            // 1) <h-side> <len> <v-side|center>
            // 2) <h-side|center> <v-side> <len>
            // 3) <v-side> <len> <h-side|center>
            // 4) <v-side|center> <h-side> <len>
            if (hs := parseHSide(toks[0])) {
                if (lp := PosLength.parse(toks[1])) {
                    if (eqIgnoreCase(toks[2], "center")) return CssPosition{ x: HorizontalPosition.fromSide(hs, lp), y: VerticalPosition.centerVal() };
                    if (vs := parseVSide(toks[2])) return CssPosition{ x: HorizontalPosition.fromSide(hs, lp), y: VerticalPosition.fromSide(vs, null) };
                    return null;
                }
                if (vs := parseVSide(toks[1])) {
                    const lp = PosLength.parse(toks[2]) ?? return null;
                    return CssPosition{ x: HorizontalPosition.fromSide(hs, null), y: VerticalPosition.fromSide(vs, lp) };
                }
                return null;
            }
            if (vs := parseVSide(toks[0])) {
                if (lp := PosLength.parse(toks[1])) {
                    if (eqIgnoreCase(toks[2], "center")) return CssPosition{ x: HorizontalPosition.centerVal(), y: VerticalPosition.fromSide(vs, lp) };
                    if (hs := parseHSide(toks[2])) return CssPosition{ x: HorizontalPosition.fromSide(hs, null), y: VerticalPosition.fromSide(vs, lp) };
                    return null;
                }
                if (hs := parseHSide(toks[1])) {
                    const lp = PosLength.parse(toks[2]) ?? return null;
                    return CssPosition{ x: HorizontalPosition.fromSide(hs, lp), y: VerticalPosition.fromSide(vs, null) };
                }
                return null;
            }
            if (eqIgnoreCase(toks[0], "center")) {
                const lp = PosLength.parse(toks[2]) ?? return null;
                if (hs := parseHSide(toks[1])) return CssPosition{ x: HorizontalPosition.fromSide(hs, lp), y: VerticalPosition.centerVal() };
                if (vs := parseVSide(toks[1])) return CssPosition{ x: HorizontalPosition.centerVal(), y: VerticalPosition.fromSide(vs, lp) };
            }
            return null;
        }

        // n == 4: (<h-side> <len> <v-side> <len>) or (<v-side> <len> <h-side> <len>)
        if (hs := parseHSide(toks[0])) {
            const x_off = PosLength.parse(toks[1]) ?? return null;
            const vs = parseVSide(toks[2]) ?? return null;
            const y_off = PosLength.parse(toks[3]) ?? return null;
            return CssPosition{
                x: HorizontalPosition.fromSide(hs, x_off),
                y: VerticalPosition.fromSide(vs, y_off),
            };
        }
        if (vs := parseVSide(toks[0])) {
            const y_off = PosLength.parse(toks[1]) ?? return null;
            const hs = parseHSide(toks[2]) ?? return null;
            const x_off = PosLength.parse(toks[3]) ?? return null;
            return CssPosition{
                x: HorizontalPosition.fromSide(hs, x_off),
                y: VerticalPosition.fromSide(vs, y_off),
            };
        }
        return null;
    }

    /// Serialize to canonical shortest CSS `<position>` string into `out`.
    pub fn toCss(out: []u8): []const u8 {
        if (_.x.isCenter() && _.y.isCenter()) {
            const l = appendStr(out, 0, "50%");
            return out[0..l];
        }
        // Simple 1- or 2-value forms without side offsets
        const x_has_off = _.x.kind == .side && _.x.has_offset && !_.x.length.isZero();
        const y_has_off = _.y.kind == .side && _.y.has_offset && !_.y.length.isZero();

        if (!x_has_off && !y_has_off) {
            let p = writeSimpleX(out, 0, _.x);
            if (!_.y.isCenter()) {
                p = appendStr(out, p, " ");
                p = writeSimpleY(out, p, _.y);
            }
            return out[0..p];
        }

        // 3/4-value form with explicit side offsets
        let p: usize = 0;
        if (_.x.kind == .side) {
            p = appendStr(out, p, _.x.side.asStr());
            if (x_has_off) {
                p = appendStr(out, p, " ");
                p = writePosLen(out, p, _.x.length);
            }
        } else if (_.x.kind == .center) {
            p = appendStr(out, p, "center");
        } else {
            p = appendStr(out, p, "left ");
            p = writePosLen(out, p, _.x.length);
        }

        p = appendStr(out, p, " ");
        if (_.y.kind == .side) {
            p = appendStr(out, p, _.y.side.asStr());
            if (y_has_off) {
                p = appendStr(out, p, " ");
                p = writePosLen(out, p, _.y.length);
            }
        } else if (_.y.kind == .center) {
            p = appendStr(out, p, "center");
        } else {
            p = appendStr(out, p, "top ");
            p = writePosLen(out, p, _.y.length);
        }
        return out[0..p];
    }
}

// ---------- Helpers ----------

fn parseHSide(s: []const u8): HorizontalSide? {
    if (eqIgnoreCase(s, "left")) return .left;
    if (eqIgnoreCase(s, "right")) return .right;
    return null;
}

fn parseVSide(s: []const u8): VerticalSide? {
    if (eqIgnoreCase(s, "top")) return .top;
    if (eqIgnoreCase(s, "bottom")) return .bottom;
    return null;
}

fn writeSimpleX(out: []u8, pos: usize, x: HorizontalPosition): usize {
    switch (x.kind) {
        .center => return appendStr(out, pos, "50%"),
        .length => return writePosLen(out, pos, x.length),
        .side => return if (x.side == .left) appendStr(out, pos, "0") else appendStr(out, pos, "100%"),
    }
}

fn writeSimpleY(out: []u8, pos: usize, y: VerticalPosition): usize {
    switch (y.kind) {
        .center => return appendStr(out, pos, "50%"),
        .length => return writePosLen(out, pos, y.length),
        .side => return if (y.side == .top) appendStr(out, pos, "0") else appendStr(out, pos, "100%"),
    }
}

fn writePosLen(out: []u8, pos: usize, l: PosLength): usize {
    if (l.value == 0.0 && l.unit != .percent) return appendStr(out, pos, "0");
    if (l.value == 0.0 && l.unit == .percent) return appendStr(out, pos, "0");
    const p = writeFloat(out, pos, l.value);
    return appendStr(out, p, l.unit.asStr());
}

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
