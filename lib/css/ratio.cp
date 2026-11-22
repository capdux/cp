// Source: Absorbed from /data/dev/bun/src/css/values/ratio.cp | Synced: 2026-11-14T18:41:18Z
// ============================================================================
// lib/css/ratio.cp — Zero-Alloc CSS Values Level 4 `<ratio>` & `aspect-ratio`
// ============================================================================
// Implements W3C CSS Values & Units Level 4 §7.2 `<ratio>` and CSS Sizing
// Level 4 `aspect-ratio` parsing, degenerate ratio detection, GCD canonical
// reduction, cross-multiplied comparison, viewport/layout dimension resolution,
// media-query aspect-ratio matching, and Bun-aligned minimal CSS serialization
// (omitting `/ 1`, normalizing `-0` to `0`, omitting leading `0` for
// `0 < x < 1`, and writing directly into caller-provided `[]u8` slices).

pub err CssRatioError {
    EmptyInput,
    InvalidSyntax,
    NegativeValue,
    MissingDelimiter,
    NotFinite,
    Overflow,
    BufferTooSmall,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn trimSpaces(raw: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = raw.len;
    while (start < end && isSpace(raw[start])) start += 1;
    while (end > start && isSpace(raw[end - 1])) end -= 1;
    return raw[start..end];
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

fn isFiniteF64(x: f64): bool {
    return x == x && x <= 1.7976931348623157e308 && x >= -1.7976931348623157e308;
}

fn normalizeZero(v: f64): f64 {
    if (v == 0.0) return 0.0;
    return v;
}

fn absF64(v: f64): f64 {
    if (v < 0.0) return -v;
    return v;
}

fn gcdU64(a_in: u64, b_in: u64): u64 {
    let a = a_in;
    let b = b_in;
    while (b != 0) {
        const t = a % b;
        a = b;
        b = t;
    }
    return a;
}

fn isExactPositiveIntU64(v: f64): bool {
    if (!isFiniteF64(v)) return false;
    if (v < 1.0 || v > 9007199254740991.0) return false;
    const iv: u64 = @as(u64, @intFromFloat(v));
    return @as(f64, @floatFromInt(iv)) == v;
}

fn appendByte(out: []u8, pos: usize, ch: u8): CssRatioError!usize {
    if (pos >= out.len) return CssRatioError.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}

fn appendSlice(out: []u8, pos: usize, src: []const u8): CssRatioError!usize {
    if (src.len > out.len - pos) return CssRatioError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeUint(out: []u8, pos: usize, val_in: u64): CssRatioError!usize {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val_in;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v /= 10;
            n += 1;
        }
    }
    if (n > out.len - pos) return CssRatioError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

/// Formats a non-negative CSS `<number>` in canonical minimal form (up to 6 fractional digits).
fn formatCssNumber(out: []u8, pos_in: usize, val_in: f64): CssRatioError!usize {
    if (!isFiniteF64(val_in)) return CssRatioError.NotFinite;
    if (val_in < 0.0) return CssRatioError.NegativeValue;

    let pos = pos_in;
    if (val_in == 0.0) {
        return appendByte(out, pos, '0');
    }
    if (val_in >= 9.0e12) return CssRatioError.Overflow;

    const scaled: u64 = @as(u64, @intFromFloat(val_in * 1000000.0 + 0.5));
    if (scaled == 0) {
        return appendByte(out, pos, '0');
    }

    const int_part: u64 = scaled / 1000000;
    let frac_part: u64 = scaled % 1000000;

    // Omit leading '0' when 0 < val < 1 (e.g. ".5", ".25")
    if (int_part > 0 || frac_part == 0) {
        pos = writeUint(out, pos, int_part)?;
    }

    if (frac_part > 0) {
        pos = appendByte(out, pos, '.')?;
        let digits: [6]u8 = [0; 6];
        let k: usize = 6;
        while (k > 0) {
            k -= 1;
            digits[k] = @as(u8, @intCast('0' + (frac_part % 10)));
            frac_part /= 10;
        }
        let frac_len: usize = 6;
        while (frac_len > 0 && digits[frac_len - 1] == '0') {
            frac_len -= 1;
        }
        if (frac_len > out.len - pos) return CssRatioError.BufferTooSmall;
        let j: usize = 0;
        while (j < frac_len) {
            out[pos + j] = digits[j];
            j += 1;
        }
        pos += frac_len;
    }

    return pos;
}

struct ParsedNumToken {
    value: f64,
    next: usize,
}

/// Parses a single CSS `<number [0,∞]>` starting at `start`, returning the value and end offset.
fn parseNonNegNumberAt(s: []const u8, start: usize): CssRatioError!ParsedNumToken {
    let i = start;
    if (i >= s.len) return CssRatioError.InvalidSyntax;

    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }
    if (i >= s.len) return CssRatioError.InvalidSyntax;

    let int_part: f64 = 0.0;
    let has_int_digits = false;
    while (i < s.len && isDigit(s[i])) {
        has_int_digits = true;
        int_part = int_part * 10.0 + @as(f64, @floatFromInt(s[i] - '0'));
        i += 1;
    }

    let frac_part: f64 = 0.0;
    let has_frac_digits = false;
    if (i < s.len && s[i] == '.') {
        i += 1;
        let scale: f64 = 0.1;
        while (i < s.len && isDigit(s[i])) {
            has_frac_digits = true;
            frac_part += @as(f64, @floatFromInt(s[i] - '0')) * scale;
            scale *= 0.1;
            i += 1;
        }
        if (!has_frac_digits) return CssRatioError.InvalidSyntax;
    }

    if (!has_int_digits && !has_frac_digits) return CssRatioError.InvalidSyntax;

    let val = int_part + frac_part;

    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        if (i >= s.len) return CssRatioError.InvalidSyntax;
        let exp_neg = false;
        if (s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (s[i] == '+') {
            i += 1;
        }
        if (i >= s.len || !isDigit(s[i])) return CssRatioError.InvalidSyntax;
        let exp_val: u32 = 0;
        let exp_overflow = false;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 10000) {
                exp_val = exp_val * 10 + @as(u32, s[i] - '0');
            } else {
                exp_overflow = true;
            }
            i += 1;
        }
        if (exp_overflow || exp_val > 308) {
            if (exp_neg) {
                val = 0.0;
            } else if (val != 0.0) {
                return CssRatioError.Overflow;
            }
        } else {
            let k: u32 = 0;
            while (k < exp_val) {
                if (exp_neg) {
                    val *= 0.1;
                } else {
                    val *= 10.0;
                }
                k += 1;
            }
        }
    }

    if (!isFiniteF64(val)) return CssRatioError.Overflow;
    if (neg && val > 0.0) return CssRatioError.NegativeValue;
    return ParsedNumToken{ value: normalizeZero(val), next: i };
}

fn parseRatioInternal(raw: []const u8, require_delim: bool): CssRatioError!CssRatioVal {
    const s = trimSpaces(raw);
    if (s.len == 0) return CssRatioError.EmptyInput;

    const first = parseNonNegNumberAt(s, 0)?;
    let i = first.next;
    while (i < s.len && isSpace(s[i])) i += 1;

    if (i == s.len) {
        if (require_delim) return CssRatioError.MissingDelimiter;
        return CssRatioVal{
            numerator: first.value,
            denominator: 1.0,
        };
    }

    if (s[i] != '/') {
        if (require_delim) return CssRatioError.MissingDelimiter;
        return CssRatioError.InvalidSyntax;
    }
    i += 1;
    while (i < s.len && isSpace(s[i])) i += 1;
    if (i >= s.len) return CssRatioError.InvalidSyntax;

    const second = parseNonNegNumberAt(s, i)?;
    let end = second.next;
    while (end < s.len && isSpace(s[end])) end += 1;
    if (end != s.len) return CssRatioError.InvalidSyntax;

    return CssRatioVal{
        numerator: first.value,
        denominator: second.value,
    };
}

/// W3C CSS Values Level 4 `<ratio>` value (`<number [0,∞]> [ / <number [0,∞]> ]?`).
pub struct CssRatioVal {
    numerator: f64 = 1.0,
    denominator: f64 = 1.0,

    /// Constructs a validated `<ratio>` from numerator and denominator.
    static fn fromParts(numerator: f64, denominator: f64): CssRatioError!CssRatioVal {
        if (!isFiniteF64(numerator) || !isFiniteF64(denominator)) {
            return CssRatioError.NotFinite;
        }
        if (numerator < 0.0 || denominator < 0.0) {
            return CssRatioError.NegativeValue;
        }
        return CssRatioVal{
            numerator: normalizeZero(numerator),
            denominator: normalizeZero(denominator),
        };
    }

    /// Constructs a validated `<ratio>` from a single number (`denominator = 1.0`).
    static fn fromNumber(n: f64): CssRatioError!CssRatioVal {
        return CssRatioVal.fromParts(n, 1.0);
    }

    /// Parses `<ratio>` where `/ <denominator>` is optional (defaults to `1.0`).
    static fn parse(raw: []const u8): CssRatioError!CssRatioVal {
        return parseRatioInternal(raw, false);
    }

    /// Parses `<ratio>` where both operands and `/` are required (e.g. `@media (aspect-ratio: 16/9)`).
    static fn parseRequired(raw: []const u8): CssRatioError!CssRatioVal {
        return parseRatioInternal(raw, true);
    }

    /// W3C CSS Values 4 §7.2: a ratio is degenerate if either operand is zero (or invalid).
    fn isDegenerate(): bool {
        if (!isFiniteF64(_.numerator) || !isFiniteF64(_.denominator)) return true;
        return _.numerator <= 0.0 || _.denominator <= 0.0;
    }

    /// Returns the quotient `numerator / denominator`, or `null` if the ratio is degenerate.
    fn value(): f64? {
        if (_.isDegenerate()) return null;
        const q = _.numerator / _.denominator;
        if (!isFiniteF64(q)) return null;
        return q;
    }

    /// Returns the reciprocal ratio `denominator / numerator`.
    fn reciprocal(): CssRatioVal {
        return CssRatioVal{
            numerator: _.denominator,
            denominator: _.numerator,
        };
    }

    /// Mirrors Bun `Ratio.addF32`: offsets the numerator while validating non-negativity.
    fn addNumerator(delta: f64): CssRatioError!CssRatioVal {
        if (!isFiniteF64(delta)) return CssRatioError.NotFinite;
        const next_num = _.numerator + delta;
        if (!isFiniteF64(next_num)) return CssRatioError.Overflow;
        if (next_num < 0.0) return CssRatioError.NegativeValue;
        return CssRatioVal{
            numerator: normalizeZero(next_num),
            denominator: _.denominator,
        };
    }

    /// Reduces positive integer ratios by their GCD (e.g., `1920 / 1080` -> `16 / 9`).
    /// Non-integer or degenerate ratios are returned unchanged.
    fn reduce(): CssRatioVal {
        if (!isExactPositiveIntU64(_.numerator) || !isExactPositiveIntU64(_.denominator)) {
            return CssRatioVal{ numerator: _.numerator, denominator: _.denominator };
        }
        const a: u64 = @as(u64, @intFromFloat(_.numerator));
        const b: u64 = @as(u64, @intFromFloat(_.denominator));
        const g = gcdU64(a, b);
        if (g <= 1) {
            return CssRatioVal{ numerator: _.numerator, denominator: _.denominator };
        }
        return CssRatioVal{
            numerator: @as(f64, @floatFromInt(a / g)),
            denominator: @as(f64, @floatFromInt(b / g)),
        };
    }

    /// Compares two ratios by cross-multiplication (`lhs.num * rhs.den` vs `rhs.num * lhs.den`).
    /// Degenerate ratios sort before non-degenerate ratios; two degenerate ratios compare equal (0).
    fn compare(other: CssRatioVal): i32 {
        const d1 = _.isDegenerate();
        const d2 = other.isDegenerate();
        if (d1 && d2) return 0;
        if (d1) return -1;
        if (d2) return 1;
        const lhs = _.numerator * other.denominator;
        const rhs = other.numerator * _.denominator;
        if (lhs < rhs) return -1;
        if (lhs > rhs) return 1;
        return 0;
    }

    /// Exact structural equality (matches Bun `Ratio.eql`).
    fn eql(other: CssRatioVal): bool {
        return _.numerator == other.numerator && _.denominator == other.denominator;
    }

    /// Mathematical equivalence within tolerance `eps` (e.g. `16/9` ~= `32/18`).
    fn equivalent(other: CssRatioVal, eps: f64): bool {
        if (_.isDegenerate() && other.isDegenerate()) return true;
        if (v1 := _.value()) {
            if (v2 := other.value()) {
                return absF64(v1 - v2) <= eps;
            }
        }
        return false;
    }

    /// Resolves height from a given width: `height = width * denominator / numerator`.
    fn resolveHeight(width: f64): f64? {
        if (!isFiniteF64(width) || width < 0.0 || _.isDegenerate()) return null;
        const h = (width * _.denominator) / _.numerator;
        if (!isFiniteF64(h)) return null;
        return normalizeZero(h);
    }

    /// Resolves width from a given height: `width = height * numerator / denominator`.
    fn resolveWidth(height: f64): f64? {
        if (!isFiniteF64(height) || height < 0.0 || _.isDegenerate()) return null;
        const w = (height * _.numerator) / _.denominator;
        if (!isFiniteF64(w)) return null;
        return normalizeZero(w);
    }

    /// Serializes `<ratio>` into `out` with zero allocation.
    /// Omits `/ 1` when `denominator == 1.0`. Uses `/` when `minify == true`, ` / ` otherwise.
    fn toCss(out: []u8, minify: bool): CssRatioError!usize {
        let pos: usize = 0;
        pos = formatCssNumber(out, pos, _.numerator)?;
        if (_.denominator != 1.0) {
            if (minify) {
                pos = appendByte(out, pos, '/')?;
            } else {
                pos = appendSlice(out, pos, " / ")?;
            }
            pos = formatCssNumber(out, pos, _.denominator)?;
        }
        return pos;
    }
}

/// Discriminant for CSS Sizing Level 4 `aspect-ratio` property values.
pub enum CssAspectRatioKind: u8 {
    Auto = 0,
    Ratio = 1,
    AutoRatio = 2,
}

/// W3C CSS Sizing Level 4 `aspect-ratio` property (`auto | <ratio> | auto && <ratio>`).
pub struct CssAspectRatioVal {
    kind: CssAspectRatioKind = CssAspectRatioKind.Auto,
    ratio: CssRatioVal = CssRatioVal{},

    static fn auto(): CssAspectRatioVal {
        return CssAspectRatioVal{
            kind: CssAspectRatioKind.Auto,
            ratio: CssRatioVal{},
        };
    }

    static fn fromRatio(r: CssRatioVal): CssAspectRatioVal {
        return CssAspectRatioVal{
            kind: CssAspectRatioKind.Ratio,
            ratio: r,
        };
    }

    static fn autoWithRatio(r: CssRatioVal): CssAspectRatioVal {
        return CssAspectRatioVal{
            kind: CssAspectRatioKind.AutoRatio,
            ratio: r,
        };
    }

    /// Parses `auto`, `<ratio>`, `auto <ratio>`, or `<ratio> auto`.
    static fn parse(raw: []const u8): CssRatioError!CssAspectRatioVal {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssRatioError.EmptyInput;

        if (eqlIgnoreCase(s, "auto")) {
            return CssAspectRatioVal.auto();
        }

        // Check leading `auto `
        if (s.len > 4 && eqlIgnoreCase(s[0..4], "auto") && isSpace(s[4])) {
            const rest = trimSpaces(s[4..]);
            if (rest.len == 0) return CssAspectRatioVal.auto();
            const r = CssRatioVal.parse(rest)?;
            return CssAspectRatioVal.autoWithRatio(r);
        }

        // Check trailing ` auto`
        if (s.len > 4 && eqlIgnoreCase(s[(s.len - 4)..], "auto") && isSpace(s[s.len - 5])) {
            const head = trimSpaces(s[0..(s.len - 4)]);
            if (head.len == 0) return CssAspectRatioVal.auto();
            const r = CssRatioVal.parse(head)?;
            return CssAspectRatioVal.autoWithRatio(r);
        }

        const r = CssRatioVal.parse(s)?;
        return CssAspectRatioVal.fromRatio(r);
    }

    fn isAuto(): bool {
        return _.kind == CssAspectRatioKind.Auto || _.kind == CssAspectRatioKind.AutoRatio;
    }

    fn hasRatio(): bool {
        return _.kind == CssAspectRatioKind.Ratio || _.kind == CssAspectRatioKind.AutoRatio;
    }

    /// Returns the active non-degenerate ratio, or `null` if pure `auto` or degenerate.
    fn effectiveRatio(): CssRatioVal? {
        if (!_.hasRatio()) return null;
        if (_.ratio.isDegenerate()) return null;
        return _.ratio;
    }

    fn eql(other: CssAspectRatioVal): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == CssAspectRatioKind.Auto) return true;
        return _.ratio.eql(other.ratio);
    }

    /// Serializes `aspect-ratio` in canonical CSS order (`auto`, `<ratio>`, or `auto <ratio>`).
    fn toCss(out: []u8, minify: bool): CssRatioError!usize {
        if (_.kind == CssAspectRatioKind.Auto) {
            return appendSlice(out, 0, "auto");
        }
        if (_.kind == CssAspectRatioKind.Ratio) {
            return _.ratio.toCss(out, minify);
        }
        let pos = appendSlice(out, 0, "auto ")?;
        const written = _.ratio.toCss(out[pos..], minify)?;
        return pos + written;
    }
}

/// Evaluates CSS Media Queries Level 4 `(aspect-ratio)`, `(min-aspect-ratio)`,
/// and `(max-aspect-ratio)` against a viewport `(viewport_w, viewport_h)`.
/// - `min_mode == true && max_mode == false`: `viewport >= target`
/// - `min_mode == false && max_mode == true`: `viewport <= target`
/// - both false (or both true): `viewport == target`
pub fn matchCssMediaAspectRatio(
    viewport_w: f64,
    viewport_h: f64,
    target: CssRatioVal,
    min_mode: bool,
    max_mode: bool,
): bool {
    if (!isFiniteF64(viewport_w) || !isFiniteF64(viewport_h)) return false;
    if (viewport_w <= 0.0 || viewport_h <= 0.0) return false;
    if (target.isDegenerate()) return false;

    const lhs = viewport_w * target.denominator;
    const rhs = target.numerator * viewport_h;
    if (min_mode && !max_mode) return lhs >= rhs;
    if (max_mode && !min_mode) return lhs <= rhs;
    return lhs == rhs;
}
