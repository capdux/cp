// Reference Only: Functional overlap with lib/css/media.cp | Kept for comparison/analysis
// Source: Absorbed from /data/dev/bun/src/css/media_query.cp | Synced: 2026-11-22T01:29:57Z (Zero-Alloc CSS Media Queries Level 4 Parser & Evaluator)

const MAX_DEPTH: usize = 32;
const FLOAT_EPS: f64 = 1.0e-9;

/// CSS Media Type (`all`, `screen`, `print`, or `unknown`).
pub enum MediaType: u8 {
    all = 0,
    screen = 1,
    print = 2,
    unknown = 3,

    fn asStr(): string {
        return match (_) {
            MediaType.all => "all",
            MediaType.screen => "screen",
            MediaType.print => "print",
            MediaType.unknown => "unknown",
        };
    }

    fn parse(raw: []const u8): MediaType {
        if (eqlIgnoreCase(raw, "all")) return MediaType.all;
        if (eqlIgnoreCase(raw, "screen")) return MediaType.screen;
        if (eqlIgnoreCase(raw, "print")) return MediaType.print;
        return MediaType.unknown;
    }
}

/// Optional prefix qualifier on a media query (`only` or `not`).
pub enum Qualifier: u8 {
    none = 0,
    only = 1,
    not = 2,

    fn asStr(): string {
        return match (_) {
            Qualifier.none => "",
            Qualifier.only => "only",
            Qualifier.not => "not",
        };
    }
}

/// CSS Media Queries Level 4 range comparison operator.
pub enum MediaComparison: u8 {
    equal = 0,
    gt = 1,
    ge = 2,
    lt = 3,
    le = 4,

    fn opposite(): MediaComparison {
        return match (_) {
            MediaComparison.equal => MediaComparison.equal,
            MediaComparison.gt => MediaComparison.lt,
            MediaComparison.ge => MediaComparison.le,
            MediaComparison.lt => MediaComparison.gt,
            MediaComparison.le => MediaComparison.ge,
        };
    }

    fn eval(lhs: f64, rhs: f64): bool {
        return match (_) {
            MediaComparison.equal => absF64(lhs - rhs) <= FLOAT_EPS,
            MediaComparison.gt => lhs > rhs + FLOAT_EPS,
            MediaComparison.ge => lhs >= rhs - FLOAT_EPS,
            MediaComparison.lt => lhs < rhs - FLOAT_EPS,
            MediaComparison.le => lhs <= rhs + FLOAT_EPS,
        };
    }
}

/// Preferred color scheme (`prefers-color-scheme`).
pub enum ColorScheme: u8 {
    light = 0,
    dark = 1,
}

/// Reduced motion preference (`prefers-reduced-motion`).
pub enum ReducedMotion: u8 {
    no_preference = 0,
    reduce = 1,
}

/// Viewport orientation (`portrait` when `height >= width`, else `landscape`).
pub enum Orientation: u8 {
    portrait = 0,
    landscape = 1,
}

/// Primary pointing device hover capability (`hover`).
pub enum HoverKind: u8 {
    none = 0,
    hover = 1,
}

/// Primary pointing device accuracy (`pointer`).
pub enum PointerKind: u8 {
    none = 0,
    coarse = 1,
    fine = 2,
}

/// Viewport and device environment used to evaluate CSS Media Queries.
pub struct MediaEnv {
    media_type: MediaType,
    width_px: f64,
    height_px: f64,
    root_font_size_px: f64,
    dppx: f64,
    color_bits: u32,
    color_scheme: ColorScheme,
    reduced_motion: ReducedMotion,
    hover: HoverKind,
    pointer: PointerKind,

    static fn desktop(): MediaEnv {
        return {
            media_type: MediaType.screen,
            width_px: 1440.0,
            height_px: 900.0,
            root_font_size_px: 16.0,
            dppx: 1.0,
            color_bits: 8,
            color_scheme: ColorScheme.light,
            reduced_motion: ReducedMotion.no_preference,
            hover: HoverKind.hover,
            pointer: PointerKind.fine,
        };
    }

    fn mobile(): MediaEnv {
        return {
            media_type: MediaType.screen,
            width_px: 390.0,
            height_px: 844.0,
            root_font_size_px: 16.0,
            dppx: 3.0,
            color_bits: 8,
            color_scheme: ColorScheme.light,
            reduced_motion: ReducedMotion.no_preference,
            hover: HoverKind.none,
            pointer: PointerKind.coarse,
        };
    }

    fn orientation(): Orientation {
        return if (_.width_px > _.height_px) Orientation.landscape else Orientation.portrait;
    }

    fn isValid(): bool {
        if (!isFinite(_.width_px) || _.width_px < 0.0) return false;
        if (!isFinite(_.height_px) || _.height_px < 0.0) return false;
        if (!isFinite(_.root_font_size_px) || _.root_font_size_px <= 0.0) return false;
        if (!isFinite(_.dppx) || _.dppx <= 0.0) return false;
        return true;
    }
}

/// Parsed summary of a single CSS media query.
pub struct MediaQuerySummary {
    qualifier: Qualifier,
    media_type: MediaType,
    has_condition: bool,
    feature_count: usize,

    /// Returns true if the query is guaranteed to always match (`all` with no qualifier/condition).
    fn alwaysMatches(): bool {
        return _.qualifier == Qualifier.none && _.media_type == MediaType.all && !_.has_condition;
    }
}

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn toLowerAscii(c: u8): u8 {
    return if (c >= 'A' && c <= 'Z') c + 32 else c;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
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

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return eqlIgnoreCase(s[0..prefix.len], prefix);
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

// Parses a decimal number prefix (optional sign, digits, optional decimal point) and returns (value, consumed_len).
struct ParsedNum {
    val: f64,
    len: usize,
}

fn parseNumberPrefix(s: []const u8): ParsedNum? {
    if (s.len == 0) return null;
    let idx: usize = 0;
    let neg = false;
    if (s[idx] == '+') {
        idx += 1;
    } else if (s[idx] == '-') {
        neg = true;
        idx += 1;
    }
    let int_part: f64 = 0.0;
    let digits: usize = 0;
    while (idx < s.len && s[idx] >= '0' && s[idx] <= '9') {
        int_part = int_part * 10.0 + ((s[idx] - '0') as f64);
        idx += 1;
        digits += 1;
    }
    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 1.0;
    if (idx < s.len && s[idx] == '.') {
        idx += 1;
        while (idx < s.len && s[idx] >= '0' && s[idx] <= '9') {
            frac_scale *= 0.1;
            frac_part += ((s[idx] - '0') as f64) * frac_scale;
            idx += 1;
            digits += 1;
        }
    }
    if (digits == 0) return null;
    const total = int_part + frac_part;
    if (!isFinite(total)) return null;
    return {
        val: if (neg) 0.0 - total else total,
        len: idx,
    };
}

/// Parses a CSS length (`px`, `em`, `rem`, `cm`, `mm`, `in`, `pt`, `pc`, or unitless `0`) into pixels.
pub fn parseLengthPx(raw: []const u8, root_font_px: f64): f64? {
    if (!isFinite(root_font_px) || root_font_px <= 0.0) return null;
    const s = trimSpaces(raw);
    if (p := parseNumberPrefix(s)) {
        const unit = trimSpaces(s[p.len..]);
        if (unit.len == 0) {
            return if (p.val == 0.0) 0.0 else null;
        }
        if (eqlIgnoreCase(unit, "px")) return p.val;
        if (eqlIgnoreCase(unit, "em") || eqlIgnoreCase(unit, "rem")) return p.val * root_font_px;
        if (eqlIgnoreCase(unit, "in")) return p.val * 96.0;
        if (eqlIgnoreCase(unit, "cm")) return p.val * (96.0 / 2.54);
        if (eqlIgnoreCase(unit, "mm")) return p.val * (96.0 / 25.4);
        if (eqlIgnoreCase(unit, "pt")) return p.val * (96.0 / 72.0);
        if (eqlIgnoreCase(unit, "pc")) return p.val * 16.0;
    }
    return null;
}

/// Parses a CSS resolution (`dppx`, `x`, `dpi`, `dpcm`) into `dppx`.
pub fn parseResolutionDppx(raw: []const u8): f64? {
    const s = trimSpaces(raw);
    if (p := parseNumberPrefix(s)) {
        if (p.val < 0.0) return null;
        const unit = trimSpaces(s[p.len..]);
        if (eqlIgnoreCase(unit, "dppx") || eqlIgnoreCase(unit, "x")) return p.val;
        if (eqlIgnoreCase(unit, "dpi")) return p.val / 96.0;
        if (eqlIgnoreCase(unit, "dpcm")) return p.val * (2.54 / 96.0);
    }
    return null;
}

/// Parses a CSS ratio (`16/9`, `16 / 9`, or positive number `1.5`) into a floating-point quotient.
pub fn parseRatio(raw: []const u8): f64? {
    const s = trimSpaces(raw);
    if (p1 := parseNumberPrefix(s)) {
        if (p1.val < 0.0) return null;
        const rest = trimSpaces(s[p1.len..]);
        if (rest.len == 0) {
            return p1.val;
        }
        if (rest[0] == '/') {
            const rhs = trimSpaces(rest[1..]);
            if (p2 := parseNumberPrefix(rhs)) {
                if (trimSpaces(rhs[p2.len..]).len != 0 || p2.val <= 0.0) return null;
                return p1.val / p2.val;
            }
        }
    }
    return null;
}

fn parseUnitlessNumber(raw: []const u8): f64? {
    const s = trimSpaces(raw);
    if (p := parseNumberPrefix(s)) {
        if (trimSpaces(s[p.len..]).len == 0) return p.val;
    }
    return null;
}

enum FeatureDomain: u8 {
    length = 0,
    ratio = 1,
    resolution = 2,
    number = 3,
    keyword = 4,
    unknown = 5,
}

fn featureDomain(name: []const u8): FeatureDomain {
    if (eqlIgnoreCase(name, "width") || eqlIgnoreCase(name, "height") || eqlIgnoreCase(name, "device-width") || eqlIgnoreCase(name, "device-height")) {
        return FeatureDomain.length;
    }
    if (eqlIgnoreCase(name, "aspect-ratio") || eqlIgnoreCase(name, "device-aspect-ratio")) {
        return FeatureDomain.ratio;
    }
    if (eqlIgnoreCase(name, "resolution")) {
        return FeatureDomain.resolution;
    }
    if (eqlIgnoreCase(name, "color") || eqlIgnoreCase(name, "-webkit-device-pixel-ratio") || eqlIgnoreCase(name, "-moz-device-pixel-ratio")) {
        return FeatureDomain.number;
    }
    if (eqlIgnoreCase(name, "orientation") || eqlIgnoreCase(name, "prefers-color-scheme") || eqlIgnoreCase(name, "prefers-reduced-motion") || eqlIgnoreCase(name, "hover") || eqlIgnoreCase(name, "pointer") || eqlIgnoreCase(name, "any-hover") || eqlIgnoreCase(name, "any-pointer")) {
        return FeatureDomain.keyword;
    }
    return FeatureDomain.unknown;
}

fn envNumericValue(name: []const u8, env: MediaEnv): f64? {
    if (eqlIgnoreCase(name, "width") || eqlIgnoreCase(name, "device-width")) return env.width_px;
    if (eqlIgnoreCase(name, "height") || eqlIgnoreCase(name, "device-height")) return env.height_px;
    if (eqlIgnoreCase(name, "aspect-ratio") || eqlIgnoreCase(name, "device-aspect-ratio")) {
        if (env.height_px <= 0.0) return null;
        return env.width_px / env.height_px;
    }
    if (eqlIgnoreCase(name, "resolution") || eqlIgnoreCase(name, "-webkit-device-pixel-ratio") || eqlIgnoreCase(name, "-moz-device-pixel-ratio")) {
        return env.dppx;
    }
    if (eqlIgnoreCase(name, "color")) return env.color_bits as f64;
    return null;
}

fn parseFeatureNumeric(domain: FeatureDomain, val_str: []const u8, env: MediaEnv): f64? {
    return match (domain) {
        FeatureDomain.length => parseLengthPx(val_str, env.root_font_size_px),
        FeatureDomain.ratio => parseRatio(val_str),
        FeatureDomain.resolution => parseResolutionDppx(val_str),
        FeatureDomain.number => parseUnitlessNumber(val_str),
        else => null,
    };
}

fn evalKeywordFeature(name: []const u8, val_str: []const u8, env: MediaEnv): bool? {
    const v = trimSpaces(val_str);
    if (eqlIgnoreCase(name, "orientation")) {
        if (eqlIgnoreCase(v, "portrait")) return env.orientation() == Orientation.portrait;
        if (eqlIgnoreCase(v, "landscape")) return env.orientation() == Orientation.landscape;
        return false;
    }
    if (eqlIgnoreCase(name, "prefers-color-scheme")) {
        if (eqlIgnoreCase(v, "light")) return env.color_scheme == ColorScheme.light;
        if (eqlIgnoreCase(v, "dark")) return env.color_scheme == ColorScheme.dark;
        return false;
    }
    if (eqlIgnoreCase(name, "prefers-reduced-motion")) {
        if (eqlIgnoreCase(v, "no-preference")) return env.reduced_motion == ReducedMotion.no_preference;
        if (eqlIgnoreCase(v, "reduce")) return env.reduced_motion == ReducedMotion.reduce;
        return false;
    }
    if (eqlIgnoreCase(name, "hover") || eqlIgnoreCase(name, "any-hover")) {
        if (eqlIgnoreCase(v, "none")) return env.hover == HoverKind.none;
        if (eqlIgnoreCase(v, "hover")) return env.hover == HoverKind.hover;
        return false;
    }
    if (eqlIgnoreCase(name, "pointer") || eqlIgnoreCase(name, "any-pointer")) {
        if (eqlIgnoreCase(v, "none")) return env.pointer == PointerKind.none;
        if (eqlIgnoreCase(v, "coarse")) return env.pointer == PointerKind.coarse;
        if (eqlIgnoreCase(v, "fine")) return env.pointer == PointerKind.fine;
        return false;
    }
    return false;
}

fn evalBooleanFeature(name: []const u8, env: MediaEnv): bool {
    if (v := envNumericValue(name, env)) {
        return v > 0.0;
    }
    if (eqlIgnoreCase(name, "hover") || eqlIgnoreCase(name, "any-hover")) {
        return env.hover != HoverKind.none;
    }
    if (eqlIgnoreCase(name, "pointer") || eqlIgnoreCase(name, "any-pointer")) {
        return env.pointer != PointerKind.none;
    }
    if (eqlIgnoreCase(name, "prefers-reduced-motion")) {
        return env.reduced_motion == ReducedMotion.reduce;
    }
    return false;
}

struct CompMatch {
    op: MediaComparison,
    start: usize,
    end: usize,
}

fn findComparison(s: []const u8, start_pos: usize): CompMatch? {
    let i = start_pos;
    while (i < s.len) {
        const c = s[i];
        if (c == '<') {
            if (i + 1 < s.len && s[i + 1] == '=') {
                return { op: MediaComparison.le, start: i, end: i + 2 };
            }
            return { op: MediaComparison.lt, start: i, end: i + 1 };
        }
        if (c == '>') {
            if (i + 1 < s.len && s[i + 1] == '=') {
                return { op: MediaComparison.ge, start: i, end: i + 2 };
            }
            return { op: MediaComparison.gt, start: i, end: i + 1 };
        }
        if (c == '=') {
            return { op: MediaComparison.equal, start: i, end: i + 1 };
        }
        i += 1;
    }
    return null;
}

fn isPureIdent(s: []const u8): bool {
    if (s.len == 0) return false;
    // Must not start with a digit (to distinguish lengths/numbers from feature names)
    if (s[0] >= '0' && s[0] <= '9') return false;
    let i: usize = 0;
    while (i < s.len) {
        if (!isIdentChar(s[i])) return false;
        i += 1;
    }
    return true;
}

// Evaluates the interior of a feature parenthesis `(...)` without outer parens.
fn evalFeatureExpr(inner_raw: []const u8, env: MediaEnv, eval_mode: bool): bool? {
    const inner = trimSpaces(inner_raw);
    if (inner.len == 0) return null;

    // 1. Traditional `name: value` syntax (including `min-*` / `max-*` prefixes)
    let colon_idx: usize? = null;
    let ci: usize = 0;
    while (ci < inner.len) {
        if (inner[ci] == ':') {
            colon_idx = ci;
            break;
        }
        ci += 1;
    }
    if (cpos := colon_idx) {
        const raw_name = trimSpaces(inner[0..cpos]);
        const val_str = trimSpaces(inner[(cpos + 1)..]);
        if (!isPureIdent(raw_name) || val_str.len == 0) return null;

        let cmp = MediaComparison.equal;
        let base_name = raw_name;
        if (startsWithIgnoreCase(raw_name, "min-")) {
            cmp = MediaComparison.ge;
            base_name = raw_name[4..];
        } else if (startsWithIgnoreCase(raw_name, "max-")) {
            cmp = MediaComparison.le;
            base_name = raw_name[4..];
        } else if (startsWithIgnoreCase(raw_name, "-webkit-min-")) {
            cmp = MediaComparison.ge;
            base_name = "-webkit-device-pixel-ratio";
            if (!eqlIgnoreCase(raw_name, "-webkit-min-device-pixel-ratio")) {
                base_name = raw_name;
            }
        } else if (startsWithIgnoreCase(raw_name, "-webkit-max-")) {
            cmp = MediaComparison.le;
            base_name = "-webkit-device-pixel-ratio";
            if (!eqlIgnoreCase(raw_name, "-webkit-max-device-pixel-ratio")) {
                base_name = raw_name;
            }
        }

        if (!eval_mode) return true;
        const dom = featureDomain(base_name);
        if (dom == FeatureDomain.keyword) {
            if (cmp != MediaComparison.equal) return null;
            return evalKeywordFeature(base_name, val_str, env);
        }
        if (dom == FeatureDomain.unknown) {
            return false;
        }
        if (target := parseFeatureNumeric(dom, val_str, env)) {
            if (actual := envNumericValue(base_name, env)) {
                return cmp.eval(actual, target);
            }
            return false;
        }
        return null;
    }

    // 2. Range syntax: `name <op> val`, `val <op> name`, or `val1 <op1> name <op2> val2`
    if (m1 := findComparison(inner, 0)) {
        const left = trimSpaces(inner[0..m1.start]);
        if (left.len == 0) return null;

        if (m2 := findComparison(inner, m1.end)) {
            // Double-sided interval: `val1 <op1> name <op2> val2`
            if (findComparison(inner, m2.end) != null) return null;
            const mid = trimSpaces(inner[m1.end..m2.start]);
            const right = trimSpaces(inner[m2.end..]);
            if (!isPureIdent(mid) || right.len == 0) return null;
            // W3C MQ4 requires both operators to point in the same direction (both < / <= or both > / >=)
            const both_lt = (m1.op == MediaComparison.lt || m1.op == MediaComparison.le) && (m2.op == MediaComparison.lt || m2.op == MediaComparison.le);
            const both_gt = (m1.op == MediaComparison.gt || m1.op == MediaComparison.ge) && (m2.op == MediaComparison.gt || m2.op == MediaComparison.ge);
            if (!both_lt && !both_gt) return null;

            if (!eval_mode) return true;
            const dom = featureDomain(mid);
            if (dom == FeatureDomain.unknown || dom == FeatureDomain.keyword) return false;
            const v1 = parseFeatureNumeric(dom, left, env);
            const v2 = parseFeatureNumeric(dom, right, env);
            if (lhs_val := v1) {
                if (rhs_val := v2) {
                    if (actual := envNumericValue(mid, env)) {
                        return m1.op.opposite().eval(actual, lhs_val) && m2.op.eval(actual, rhs_val);
                    }
                    return false;
                }
            }
            return null;
        }

        // Single comparison
        const right = trimSpaces(inner[m1.end..]);
        if (right.len == 0) return null;
        if (isPureIdent(left)) {
            if (!eval_mode) return true;
            const dom = featureDomain(left);
            if (dom == FeatureDomain.unknown || dom == FeatureDomain.keyword) return false;
            if (target := parseFeatureNumeric(dom, right, env)) {
                if (actual := envNumericValue(left, env)) {
                    return m1.op.eval(actual, target);
                }
                return false;
            }
            return null;
        }
        if (isPureIdent(right)) {
            if (!eval_mode) return true;
            const dom = featureDomain(right);
            if (dom == FeatureDomain.unknown || dom == FeatureDomain.keyword) return false;
            if (target := parseFeatureNumeric(dom, left, env)) {
                if (actual := envNumericValue(right, env)) {
                    return m1.op.opposite().eval(actual, target);
                }
                return false;
            }
            return null;
        }
        return null;
    }

    // 3. Boolean context feature: `(color)`, `(hover)`, `(width)`
    if (!isPureIdent(inner)) return null;
    if (!eval_mode) return true;
    return evalBooleanFeature(inner, env);
}

struct Cursor {
    src: []const u8,
    pos: usize,

    fn skipWs(*_) {
        while (_.pos < _.src.len && isSpace(_.src[_.pos])) {
            _.pos += 1;
        }
    }

    fn peekByte(): u8? {
        return if (_.pos < _.src.len) _.src[_.pos] else null;
    }

    fn readIdent(*_): []const u8 {
        _.skipWs();
        const start = _.pos;
        while (_.pos < _.src.len && isIdentChar(_.src[_.pos])) {
            _.pos += 1;
        }
        return _.src[start.._.pos];
    }

    fn consumeKeyword(*_, kw: []const u8): bool {
        const saved = _.pos;
        const tok = _.readIdent();
        if (eqlIgnoreCase(tok, kw)) {
            return true;
        }
        _.pos = saved;
        return false;
    }

    // Extracts balanced `(...)` interior starting at `(`
    fn readParenInner(*_): ([]const u8)? {
        _.skipWs();
        if (_.pos >= _.src.len || _.src[_.pos] != '(') return null;
        _.pos += 1;
        const start = _.pos;
        let depth: usize = 1;
        while (_.pos < _.src.len && depth > 0) {
            if (_.src[_.pos] == '(') {
                depth += 1;
            } else if (_.src[_.pos] == ')') {
                depth -= 1;
                if (depth == 0) {
                    const inner = _.src[start.._.pos];
                    _.pos += 1;
                    return inner;
                }
            }
            _.pos += 1;
        }
        return null;
    }
}

fn evalParenGroup(inner: []const u8, env: MediaEnv, depth: usize, feat_cnt: *usize, eval_mode: bool): bool? {
    if (depth >= MAX_DEPTH) return null;
    const trimmed = trimSpaces(inner);
    if (trimmed.len == 0) return null;

    // Check if nested media condition: starts with `(` or `not `
    if (trimmed[0] == '(' || startsWithIgnoreCase(trimmed, "not ") || startsWithIgnoreCase(trimmed, "not(")) {
        let sub: Cursor = { src: trimmed, pos: 0 };
        if (res := evalCondition(*sub, env, true, depth + 1, feat_cnt, eval_mode)) {
            sub.skipWs();
            if (sub.pos != sub.src.len) return null;
            return res;
        }
        return null;
    }

    feat_cnt.* += 1;
    return evalFeatureExpr(trimmed, env, eval_mode);
}

fn evalCondition(cur: *Cursor, env: MediaEnv, allow_or: bool, depth: usize, feat_cnt: *usize, eval_mode: bool): bool? {
    if (depth >= MAX_DEPTH) return null;
    cur.*.skipWs();

    // Case 1: `not ( ... )`
    if (cur.*.consumeKeyword("not")) {
        if (inner := cur.*.readParenInner()) {
            if (val := evalParenGroup(inner, env, depth + 1, feat_cnt, eval_mode)) {
                return !val;
            }
        }
        return null;
    }

    // Case 2: `( ... )` optionally followed by `and ...` or `or ...` (cannot mix `and` & `or` at same level per W3C MQ4)
    let first_val = false;
    if (inner := cur.*.readParenInner()) {
        if (v := evalParenGroup(inner, env, depth + 1, feat_cnt, eval_mode)) {
            first_val = v;
        } else {
            return null;
        }
    } else {
        return null;
    }

    let acc = first_val;
    let op_kind: u8 = 0; // 0 = unset, 1 = and, 2 = or
    while (true) {
        cur.*.skipWs();
        if (cur.*.pos >= cur.*.src.len) break;
        if (cur.*.consumeKeyword("and")) {
            if (op_kind == 2) return null;
            op_kind = 1;
            if (next_inner := cur.*.readParenInner()) {
                if (nv := evalParenGroup(next_inner, env, depth + 1, feat_cnt, eval_mode)) {
                    acc = acc && nv;
                } else {
                    return null;
                }
            } else {
                return null;
            }
        } else if (cur.*.consumeKeyword("or")) {
            if (!allow_or || op_kind == 1) return null;
            op_kind = 2;
            if (next_inner := cur.*.readParenInner()) {
                if (nv := evalParenGroup(next_inner, env, depth + 1, feat_cnt, eval_mode)) {
                    acc = acc || nv;
                } else {
                    return null;
                }
            } else {
                return null;
            }
        } else {
            return null;
        }
    }
    return acc;
}

struct SingleQueryEval {
    summary: MediaQuerySummary,
    matched: bool,
}

fn processSingleQuery(raw: []const u8, env: MediaEnv, eval_mode: bool): SingleQueryEval? {
    const trimmed = trimSpaces(raw);
    if (trimmed.len == 0) return null;

    let cur: Cursor = { src: trimmed, pos: 0 };
    let qual = Qualifier.none;
    let mtype = MediaType.all;
    let has_cond = false;
    let feat_cnt: usize = 0;
    let type_match = true;
    let cond_match = true;

    cur.skipWs();
    if (cur.peekByte() == '(') {
        // Pure media condition: `(min-width: 768px) or (orientation: landscape)`
        has_cond = true;
        if (cv := evalCondition(*cur, env, true, 0, *feat_cnt, eval_mode)) {
            cond_match = cv;
        } else {
            return null;
        }
    } else {
        const saved = cur.pos;
        if (cur.consumeKeyword("not")) {
            cur.skipWs();
            if (cur.peekByte() == '(') {
                // `not (...)` is a media condition without explicit media type
                cur.pos = saved;
                has_cond = true;
                if (cv := evalCondition(*cur, env, true, 0, *feat_cnt, eval_mode)) {
                    cond_match = cv;
                } else {
                    return null;
                }
            } else {
                qual = Qualifier.not;
                const mt_tok = cur.readIdent();
                if (mt_tok.len == 0 || eqlIgnoreCase(mt_tok, "and") || eqlIgnoreCase(mt_tok, "or") || eqlIgnoreCase(mt_tok, "not") || eqlIgnoreCase(mt_tok, "only")) {
                    return null;
                }
                mtype = MediaType.parse(mt_tok);
                type_match = (mtype == MediaType.all) || (mtype != MediaType.unknown && mtype == env.media_type);
                cur.skipWs();
                if (cur.pos < cur.src.len) {
                    if (!cur.consumeKeyword("and")) return null;
                    has_cond = true;
                    if (cv := evalCondition(*cur, env, false, 0, *feat_cnt, eval_mode)) {
                        cond_match = cv;
                    } else {
                        return null;
                    }
                }
            }
        } else {
            if (cur.consumeKeyword("only")) {
                qual = Qualifier.only;
            }
            const mt_tok = cur.readIdent();
            if (mt_tok.len == 0 || eqlIgnoreCase(mt_tok, "and") || eqlIgnoreCase(mt_tok, "or") || eqlIgnoreCase(mt_tok, "not") || eqlIgnoreCase(mt_tok, "only")) {
                return null;
            }
            mtype = MediaType.parse(mt_tok);
            type_match = (mtype == MediaType.all) || (mtype != MediaType.unknown && mtype == env.media_type);
            cur.skipWs();
            if (cur.pos < cur.src.len) {
                if (!cur.consumeKeyword("and")) return null;
                has_cond = true;
                if (cv := evalCondition(*cur, env, false, 0, *feat_cnt, eval_mode)) {
                    cond_match = cv;
                } else {
                    return null;
                }
            }
        }
    }

    cur.skipWs();
    if (cur.pos != cur.src.len) return null;

    let final_match = type_match && cond_match;
    if (qual == Qualifier.not) {
        final_match = !final_match;
    }

    return {
        summary: {
            qualifier: qual,
            media_type: mtype,
            has_condition: has_cond,
            feature_count: feat_cnt,
        },
        matched: final_match,
    };
}

/// Evaluates a CSS Media Query List (comma-separated, OR semantics) against `env`.
/// Empty input evaluates to `true` per W3C Media Queries Level 4.
/// Returns `null` on invalid environment or malformed query syntax.
pub fn evalMediaQueryList(input: []const u8, env: MediaEnv): bool? {
    if (!env.isValid()) return null;
    const trimmed = trimSpaces(input);
    if (trimmed.len == 0) return true;

    let any_match = false;
    let start: usize = 0;
    let depth: usize = 0;
    let i: usize = 0;
    while (i <= trimmed.len) {
        const at_end = (i == trimmed.len);
        if (!at_end) {
            if (trimmed[i] == '(') {
                depth += 1;
            } else if (trimmed[i] == ')') {
                if (depth == 0) return null;
                depth -= 1;
            }
        }
        if (at_end || (trimmed[i] == ',' && depth == 0)) {
            const part = trimmed[start..i];
            if (res := processSingleQuery(part, env, true)) {
                if (res.matched) {
                    any_match = true;
                }
            } else {
                return null;
            }
            start = i + 1;
        }
        i += 1;
    }
    if (depth != 0) return null;
    return any_match;
}

/// Parses a single CSS media query and returns its structural summary (`qualifier`, `media_type`, `feature_count`).
pub fn parseMediaQuery(input: []const u8): MediaQuerySummary? {
    const dummy_env = MediaEnv.desktop();
    if (res := processSingleQuery(input, dummy_env, false)) {
        return res.summary;
    }
    return null;
}

/// Zero-allocation canonicalizer for a CSS media query list (lowercases ASCII, normalizes whitespace and comma spacing).
/// Returns `null` on invalid query syntax or insufficient output buffer.
pub fn canonicalizeMediaQuery(input: []const u8, out: *[]u8): ([]const u8)? {
    const trimmed = trimSpaces(input);
    if (trimmed.len == 0) {
        if (out.*.len < 3) return null;
        out.*[0] = 'a';
        out.*[1] = 'l';
        out.*[2] = 'l';
        return out.*[0..3];
    }
    // Validate syntax first
    if (evalMediaQueryList(trimmed, MediaEnv.desktop()) == null) {
        return null;
    }

    let w: usize = 0;
    let prev_space = false;
    let i: usize = 0;
    while (i < trimmed.len) {
        const c = trimmed[i];
        if (isSpace(c)) {
            prev_space = true;
        } else if (c == ',') {
            if (w + 2 > out.*.len) return null;
            out.*[w] = ',';
            out.*[w + 1] = ' ';
            w += 2;
            prev_space = false;
        } else if (c == '(') {
            if (prev_space && w > 0 && out.*[w - 1] != '(' && out.*[w - 1] != ' ') {
                if (w >= out.*.len) return null;
                out.*[w] = ' ';
                w += 1;
            }
            if (w >= out.*.len) return null;
            out.*[w] = '(';
            w += 1;
            prev_space = false;
        } else if (c == ')') {
            if (w >= out.*.len) return null;
            out.*[w] = ')';
            w += 1;
            prev_space = false;
        } else if (c == ':') {
            if (w + 2 > out.*.len) return null;
            out.*[w] = ':';
            out.*[w + 1] = ' ';
            w += 2;
            prev_space = false;
        } else {
            if (prev_space && w > 0 && out.*[w - 1] != '(' && out.*[w - 1] != ' ') {
                if (w >= out.*.len) return null;
                out.*[w] = ' ';
                w += 1;
            }
            if (w >= out.*.len) return null;
            out.*[w] = toLowerAscii(c);
            w += 1;
            prev_space = false;
        }
        i += 1;
    }
    return out.*[0..w];
}
