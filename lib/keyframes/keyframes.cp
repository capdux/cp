// Source: Absorbed from /data/dev/bun/src/css/rules/keyframes.cp | Synced: 2026-11-22T12:49:17Z (Zero-Alloc CSS @keyframes Parser, Minifier & Timeline Sampler)

/// Vendor prefix of a `@keyframes` at-rule.
pub enum VendorPrefix: u8 {
    none = 0,
    webkit = 1,
    moz = 2,
    o = 3,

    fn atRuleStr(): string {
        return match (_) {
            VendorPrefix.none => "@keyframes",
            VendorPrefix.webkit => "@-webkit-keyframes",
            VendorPrefix.moz => "@-moz-keyframes",
            VendorPrefix.o => "@-o-keyframes",
        };
    }
}

/// Kind of `@keyframes` name token (`<custom-ident>` vs `<string>`).
pub enum KeyframesNameKind: u8 {
    ident = 0,
    string_lit = 1,
}

/// Parsed `@keyframes` animation name (mirrors Bun `KeyframesName`).
pub struct KeyframesName {
    kind: KeyframesNameKind,
    value: []const u8,

    /// Parses `<custom-ident>` or quoted `<string>`.
    /// Rejects unquoted CSS-wide keywords (`none`, `initial`, `inherit`, `unset`, `default`, `revert`, `revert-layer`).
    static fn parse(raw: []const u8): KeyframesName? {
        const s = trimSpaces(raw);
        if (s.len == 0) return null;

        if (s[0] == '"' || s[0] == '\'') {
            const q = s[0];
            if (s.len < 2 || s[s.len - 1] != q) return null;
            const inner = s[1..(s.len - 1)];
            if (inner.len == 0) return null;
            return { kind: KeyframesNameKind.string_lit, value: inner };
        }

        if (!isSimpleIdent(s) || isReservedCssKeyword(s)) {
            return null;
        }
        return { kind: KeyframesNameKind.ident, value: s };
    }

    /// Formats the name into `out`. When `minify == true`, unquotes string names that are safe non-reserved identifiers.
    fn format(minify: bool, out: *[]u8): ([]const u8)? {
        const must_quote = (_.kind == KeyframesNameKind.string_lit) && (
            !minify || !isSimpleIdent(_.value) || isReservedCssKeyword(_.value)
        );
        if (must_quote) {
            if (_.value.len + 2 > out.*.len) return null;
            out.*[0] = '"';
            let i: usize = 0;
            while (i < _.value.len) {
                out.*[1 + i] = _.value[i];
                i += 1;
            }
            out.*[1 + _.value.len] = '"';
            return out.*[0..(_.value.len + 2)];
        }
        if (_.value.len > out.*.len) return null;
        let i: usize = 0;
        while (i < _.value.len) {
            out.*[i] = _.value[i];
            i += 1;
        }
        return out.*[0.._.value.len];
    }
}

/// Kind of keyframe selector (`from`, `to`, or `<percentage>`).
pub enum SelectorKind: u8 {
    from_kw = 0,
    to_kw = 1,
    percentage = 2,
}

/// Single keyframe selector (`from`, `to`, or `0%..100%`) mirroring Bun `KeyframeSelector`.
pub struct KeyframeSelector {
    kind: SelectorKind,
    pct: f64,

    /// Parses `from`, `to`, or a percentage in `[0%, 100%]`.
    static fn parse(raw: []const u8): KeyframeSelector? {
        const s = trimSpaces(raw);
        if (s.len == 0) return null;
        if (eqlIgnoreCase(s, "from")) {
            return { kind: SelectorKind.from_kw, pct: 0.0 };
        }
        if (eqlIgnoreCase(s, "to")) {
            return { kind: SelectorKind.to_kw, pct: 100.0 };
        }
        if (s[s.len - 1] != '%') return null;
        if (v := parseNonNegDecimal(s[0..(s.len - 1)])) {
            if (v < 0.0 || v > 100.0) return null;
            return { kind: SelectorKind.percentage, pct: v };
        }
        return null;
    }

    /// Returns normalized animation timeline offset in `[0.0, 1.0]`.
    fn offset(): f64 {
        return _.pct * 0.01;
    }

    /// Formats selector into `out`. Follows Bun minification rules: `from` -> `0%`, `100%` -> `to`.
    fn format(minify: bool, out: *[]u8): ([]const u8)? {
        match (_.kind) {
            SelectorKind.from_kw => {
                const txt: []const u8 = if (minify) "0%" else "from";
                return copyInto(out, txt);
            },
            SelectorKind.to_kw => {
                return copyInto(out, "to");
            },
            SelectorKind.percentage => {
                if (minify && absF64(_.pct - 100.0) <= 1.0e-9) {
                    return copyInto(out, "to");
                }
                if (minify && absF64(_.pct) <= 1.0e-9) {
                    return copyInto(out, "0%");
                }
                return formatPercentage(_.pct, out);
            },
        }
    }
}

/// Flattened keyframe stop with normalized offset `[0.0, 1.0]` and zero-copy declaration body slice.
pub struct KeyframeStop {
    offset: f64,
    selector_raw: []const u8,
    body_raw: []const u8,
}

/// Parsed summary of a `@keyframes` rule.
pub struct ParsedKeyframes {
    vendor: VendorPrefix,
    name: KeyframesName,
    stop_count: usize,
}

/// Active interpolation segment returned by `sampleKeyframeSegment`.
pub struct KeyframeSegment {
    prev_idx: usize,
    next_idx: usize,
    local_t: f64,
}

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn toLowerAscii(c: u8): u8 {
    return if (c >= 'A' && c <= 'Z') c + 32 else c;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn isSimpleIdent(s: []const u8): bool {
    if (s.len == 0) return false;
    const c0 = s[0];
    if (c0 >= '0' && c0 <= '9') return false;
    if (c0 == '-' && s.len >= 2 && s[1] >= '0' && s[1] <= '9') return false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) return false;
        i += 1;
    }
    return true;
}

/// Returns `true` if `name` is a CSS-wide reserved keyword forbidden as an unquoted `@keyframes` name.
pub fn isReservedCssKeyword(name: []const u8): bool {
    return eqlIgnoreCase(name, "none") ||
        eqlIgnoreCase(name, "initial") ||
        eqlIgnoreCase(name, "inherit") ||
        eqlIgnoreCase(name, "unset") ||
        eqlIgnoreCase(name, "default") ||
        eqlIgnoreCase(name, "revert") ||
        eqlIgnoreCase(name, "revert-layer");
}

fn parseNonNegDecimal(s: []const u8): f64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    if (s[0] == '+') {
        i = 1;
    }
    let int_part: f64 = 0.0;
    let digits: usize = 0;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        int_part = int_part * 10.0 + ((s[i] - '0') as f64);
        i += 1;
        digits += 1;
    }
    let frac: f64 = 0.0;
    let scale: f64 = 1.0;
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            scale *= 0.1;
            frac += ((s[i] - '0') as f64) * scale;
            i += 1;
            digits += 1;
        }
    }
    if (digits == 0 || i != s.len) return null;
    const total = int_part + frac;
    return if (isFinite(total)) total else null;
}

fn copyInto(out: *[]u8, s: []const u8): ([]const u8)? {
    if (s.len > out.*.len) return null;
    let i: usize = 0;
    while (i < s.len) {
        out.*[i] = s[i];
        i += 1;
    }
    return out.*[0..s.len];
}

fn writeUint(out: *[]u8, pos: *usize, v: u64): bool {
    let tmp: [20]u8 = [0; 20];
    let n: usize = 0;
    let cur = v;
    if (cur == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (cur > 0) {
            tmp[n] = ('0' as u8) + ((cur % 10) as u8);
            cur = cur / 10;
            n += 1;
        }
    }
    if (pos.* + n > out.*.len) return false;
    let i: usize = 0;
    while (i < n) {
        out.*[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

fn formatPercentage(pct: f64, out: *[]u8): ([]const u8)? {
    // Round to at most 2 decimal places cleanly
    const scaled = @round(pct * 100.0) as u64;
    const whole = scaled / 100;
    const frac = scaled % 100;
    let pos: usize = 0;
    if (!writeUint(out, *pos, whole)) return null;
    if (frac > 0) {
        if (pos + 1 > out.*.len) return null;
        out.*[pos] = '.';
        pos += 1;
        const d1 = frac / 10;
        const d2 = frac % 10;
        if (pos + 1 > out.*.len) return null;
        out.*[pos] = ('0' as u8) + (d1 as u8);
        pos += 1;
        if (d2 > 0) {
            if (pos + 1 > out.*.len) return null;
            out.*[pos] = ('0' as u8) + (d2 as u8);
            pos += 1;
        }
    }
    if (pos + 1 > out.*.len) return null;
    out.*[pos] = '%';
    pos += 1;
    return out.*[0..pos];
}

fn skipWsAndComments(src: []const u8, pos: *usize): bool {
    while (pos.* < src.len) {
        if (isSpace(src[pos.*])) {
            pos.* += 1;
        } else if (pos.* + 1 < src.len && src[pos.*] == '/' && src[pos.* + 1] == '*') {
            pos.* += 2;
            let closed = false;
            while (pos.* + 1 < src.len) {
                if (src[pos.*] == '*' && src[pos.* + 1] == '/') {
                    pos.* += 2;
                    closed = true;
                    break;
                }
                pos.* += 1;
            }
            if (!closed) return false;
        } else {
            break;
        }
    }
    return true;
}

/// Parses a complete `@keyframes` rule (including vendor-prefixed `@-webkit-keyframes`, `@-moz-keyframes`, `@-o-keyframes`),
/// flattening comma-separated keyframe selectors into `stops_out` without heap allocation.
pub fn parseKeyframesRule(css_text: []const u8, stops_out: *[]KeyframeStop): ParsedKeyframes? {
    let pos: usize = 0;
    if (!skipWsAndComments(css_text, *pos)) return null;
    if (pos >= css_text.len || css_text[pos] != '@') return null;

    const at_start = pos;
    pos += 1;
    while (pos < css_text.len && !isSpace(css_text[pos]) && css_text[pos] != '{') {
        pos += 1;
    }
    const at_kw = css_text[at_start..pos];
    let vendor = VendorPrefix.none;
    if (eqlIgnoreCase(at_kw, "@keyframes")) {
        vendor = VendorPrefix.none;
    } else if (eqlIgnoreCase(at_kw, "@-webkit-keyframes")) {
        vendor = VendorPrefix.webkit;
    } else if (eqlIgnoreCase(at_kw, "@-moz-keyframes")) {
        vendor = VendorPrefix.moz;
    } else if (eqlIgnoreCase(at_kw, "@-o-keyframes")) {
        vendor = VendorPrefix.o;
    } else {
        return null;
    }

    if (!skipWsAndComments(css_text, *pos)) return null;
    const name_start = pos;
    while (pos < css_text.len && css_text[pos] != '{') {
        pos += 1;
    }
    if (pos >= css_text.len) return null;
    const parsed_name = KeyframesName.parse(css_text[name_start..pos]);
    let name_val: KeyframesName = { kind: KeyframesNameKind.ident, value: "" };
    if (nv := parsed_name) {
        name_val = nv;
    } else {
        return null;
    }

    // Consume outer '{'
    pos += 1;
    let stop_cnt: usize = 0;

    while (true) {
        if (!skipWsAndComments(css_text, *pos)) return null;
        if (pos >= css_text.len) return null;
        if (css_text[pos] == '}') {
            pos += 1;
            break;
        }

        // Read comma-separated selector list up to '{'
        const sel_list_start = pos;
        while (pos < css_text.len && css_text[pos] != '{' && css_text[pos] != '}') {
            pos += 1;
        }
        if (pos >= css_text.len || css_text[pos] != '{') return null;
        const sel_list = css_text[sel_list_start..pos];

        // Read declaration block up to matching '}'
        pos += 1;
        const body_start = pos;
        let brace_depth: usize = 1;
        while (pos < css_text.len && brace_depth > 0) {
            if (css_text[pos] == '{') {
                brace_depth += 1;
            } else if (css_text[pos] == '}') {
                brace_depth -= 1;
                if (brace_depth == 0) break;
            }
            pos += 1;
        }
        if (brace_depth != 0) return null;
        const body_raw = trimSpaces(css_text[body_start..pos]);
        pos += 1;

        // Split sel_list by ',' and validate every selector
        let s_start: usize = 0;
        let si: usize = 0;
        let block_sels: usize = 0;
        while (si <= sel_list.len) {
            if (si == sel_list.len || sel_list[si] == ',') {
                const raw_sel = trimSpaces(sel_list[s_start..si]);
                if (ks := KeyframeSelector.parse(raw_sel)) {
                    if (stop_cnt >= stops_out.*.len) return null;
                    stops_out.*[stop_cnt] = {
                        offset: ks.offset(),
                        selector_raw: raw_sel,
                        body_raw: body_raw,
                    };
                    stop_cnt += 1;
                    block_sels += 1;
                } else {
                    return null;
                }
                s_start = si + 1;
            }
            si += 1;
        }
        if (block_sels == 0) return null;
    }

    if (!skipWsAndComments(css_text, *pos)) return null;
    if (pos != css_text.len) return null;

    return {
        vendor: vendor,
        name: name_val,
        stop_count: stop_cnt,
    };
}

/// Stable in-place insertion sort of `stops` by ascending timeline `offset` in `[0.0, 1.0]`.
pub fn sortStopsByOffset(stops: *[]KeyframeStop) {
    let i: usize = 1;
    while (i < stops.*.len) {
        const key = stops.*[i];
        let j = i;
        while (j > 0 && key.offset < stops.*[j - 1].offset) {
            stops.*[j] = stops.*[j - 1];
            j -= 1;
        }
        stops.*[j] = key;
        i += 1;
    }
}

/// Samples the active keyframe segment `(prev_idx, next_idx, local_t)` for sorted `stops` at progress `t ∈ [0.0, 1.0]`.
/// Returns `null` on empty `stops` or non-finite/out-of-range `t`.
pub fn sampleKeyframeSegment(stops: []const KeyframeStop, t: f64): KeyframeSegment? {
    const n = stops.len;
    if (n == 0 || !isFinite(t) || t < 0.0 || t > 1.0) return null;
    if (n == 1 || t <= stops[0].offset) {
        return { prev_idx: 0, next_idx: 0, local_t: 0.0 };
    }
    if (t >= stops[n - 1].offset) {
        return { prev_idx: n - 1, next_idx: n - 1, local_t: 1.0 };
    }
    let i: usize = 0;
    while (i + 1 < n) {
        const a = stops[i].offset;
        const b = stops[i + 1].offset;
        if (t >= a && t <= b) {
            const span = b - a;
            const local_t = if (span > 1.0e-12) (t - a) / span else 0.0;
            return { prev_idx: i, next_idx: i + 1, local_t: local_t };
        }
        i += 1;
    }
    return { prev_idx: n - 1, next_idx: n - 1, local_t: 1.0 };
}
