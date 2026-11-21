// Source: Absorbed from /data/dev/bun/src/css/rules/media.cp | Synced: 2026-11-21T17:05:36Z

/// CSS Media Queries Level 4 解析错误集
pub err CssMediaError {
    InvalidMediaType,
    InvalidMediaFeature,
    InvalidFeatureValue,
    InvalidMediaQuery,
    TooManyFeatures,
    TooManyQueries,
    BufferTooSmall,
}

fn trimAscii(s: string): string {
    return std.mem.trim(u8, s, " \t\r\n");
}

/// 媒体类型（`all` / `screen` / `print`）
pub enum MediaType: u8 {
    all = 0,
    screen = 1,
    print = 2,

    static fn default(): MediaType {
        return .all;
    }

    static fn parse(input: string): MediaType? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "all")) { return .all; }
        if (std.ascii.eqlIgnoreCase(s, "screen")) { return .screen; }
        if (std.ascii.eqlIgnoreCase(s, "print")) { return .print; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .all => "all",
            .screen => "screen",
            .print => "print",
        };
    }
}

/// 媒体查询限定符（`none` / `only` / `not`）
pub enum MediaQualifier: u8 {
    none = 0,
    only = 1,
    not_kw = 2,

    static fn parse(input: string): MediaQualifier? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "only")) { return .only; }
        if (std.ascii.eqlIgnoreCase(s, "not")) { return .not_kw; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .none => "",
            .only => "only",
            .not_kw => "not",
        };
    }
}

/// `prefers-color-scheme` 配色偏好
pub enum ColorScheme: u8 {
    light = 0,
    dark = 1,

    static fn parse(input: string): ColorScheme? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "light")) { return .light; }
        if (std.ascii.eqlIgnoreCase(s, "dark")) { return .dark; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .light => "light",
            .dark => "dark",
        };
    }
}

/// `prefers-reduced-motion` 动效减弱偏好
pub enum ReducedMotion: u8 {
    no_preference = 0,
    reduce = 1,

    static fn parse(input: string): ReducedMotion? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "no-preference")) { return .no_preference; }
        if (std.ascii.eqlIgnoreCase(s, "reduce")) { return .reduce; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .no_preference => "no-preference",
            .reduce => "reduce",
        };
    }
}

/// `hover` 悬停能力
pub enum HoverCapability: u8 {
    none = 0,
    hover = 1,

    static fn parse(input: string): HoverCapability? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "none")) { return .none; }
        if (std.ascii.eqlIgnoreCase(s, "hover")) { return .hover; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .none => "none",
            .hover => "hover",
        };
    }
}

/// `pointer` 指针精度能力
pub enum PointerCapability: u8 {
    none = 0,
    coarse = 1,
    fine = 2,

    static fn parse(input: string): PointerCapability? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "none")) { return .none; }
        if (std.ascii.eqlIgnoreCase(s, "coarse")) { return .coarse; }
        if (std.ascii.eqlIgnoreCase(s, "fine")) { return .fine; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .none => "none",
            .coarse => "coarse",
            .fine => "fine",
        };
    }
}

/// 视口方向
pub enum MediaOrientation: u8 {
    portrait = 0,
    landscape = 1,

    static fn parse(input: string): MediaOrientation? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "portrait")) { return .portrait; }
        if (std.ascii.eqlIgnoreCase(s, "landscape")) { return .landscape; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .portrait => "portrait",
            .landscape => "landscape",
        };
    }
}

/// 媒体特征标识
pub enum MediaFeatureId: u8 {
    width = 0,
    height = 1,
    orientation = 2,
    prefers_color_scheme = 3,
    prefers_reduced_motion = 4,
    hover = 5,
    pointer = 6,

    static fn parse(input: string): MediaFeatureId? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "width")) { return .width; }
        if (std.ascii.eqlIgnoreCase(s, "height")) { return .height; }
        if (std.ascii.eqlIgnoreCase(s, "orientation")) { return .orientation; }
        if (std.ascii.eqlIgnoreCase(s, "prefers-color-scheme")) { return .prefers_color_scheme; }
        if (std.ascii.eqlIgnoreCase(s, "prefers-reduced-motion")) { return .prefers_reduced_motion; }
        if (std.ascii.eqlIgnoreCase(s, "hover")) { return .hover; }
        if (std.ascii.eqlIgnoreCase(s, "pointer")) { return .pointer; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .width => "width",
            .height => "height",
            .orientation => "orientation",
            .prefers_color_scheme => "prefers-color-scheme",
            .prefers_reduced_motion => "prefers-reduced-motion",
            .hover => "hover",
            .pointer => "pointer",
        };
    }
}

/// 范围比较运算符
pub enum CompareOp: u8 {
    eq = 0,
    ge = 1,
    le = 2,
    gt = 3,
    lt = 4,
}

/// 媒体查询求值环境（视口尺寸、设备类型与用户偏好）
pub struct MediaEnv {
    media_type: MediaType = MediaType.screen,
    width: f64 = 1280.0,
    height: f64 = 720.0,
    color_scheme: ColorScheme = ColorScheme.light,
    reduced_motion: ReducedMotion = ReducedMotion.no_preference,
    hover: HoverCapability = HoverCapability.hover,
    pointer: PointerCapability = PointerCapability.fine,
}

fn parseLengthPx(raw: string): f64 ! CssMediaError {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return err.InvalidFeatureValue;
    }
    let scale: f64 = 1.0;
    let num_part = s;
    if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "px")) {
        num_part = s[0 .. s.len - 2];
    } else if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "rem")) {
        num_part = s[0 .. s.len - 3];
        scale = 16.0;
    } else if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "em")) {
        num_part = s[0 .. s.len - 2];
        scale = 16.0;
    } else {
        const zero_v = std.fmt.parseFloat(f64, s) catch return err.InvalidFeatureValue;
        if (zero_v == 0.0) {
            return 0.0;
        }
        return err.InvalidFeatureValue;
    }
    const v = std.fmt.parseFloat(f64, num_part) catch return err.InvalidFeatureValue;
    if (std.math.isNan(v) || std.math.isInf(v) || v < 0.0) {
        return err.InvalidFeatureValue;
    }
    return v * scale;
}

fn compareValues(actual: f64, expected: f64, op: CompareOp): bool {
    const eps: f64 = 0.000001;
    return switch (op) {
        .eq => {
            const diff = if (actual > expected) (actual - expected) else (expected - actual);
            return diff <= eps;
        },
        .ge => actual + eps >= expected,
        .le => actual <= expected + eps,
        .gt => actual > expected + eps,
        .lt => actual + eps < expected,
    };
}

/// 单个媒体特征条件（如 `(min-width: 768px)`、`(prefers-color-scheme: dark)`、`(width >= 1024px)`）
pub struct MediaFeature {
    feature: MediaFeatureId,
    op: CompareOp = CompareOp.eq,
    px_value: f64 = 0.0,
    orientation: MediaOrientation = MediaOrientation.landscape,
    color_scheme: ColorScheme = ColorScheme.light,
    reduced_motion: ReducedMotion = ReducedMotion.no_preference,
    hover: HoverCapability = HoverCapability.hover,
    pointer: PointerCapability = PointerCapability.fine,

    static fn parse(input: string): MediaFeature ! CssMediaError {
        let s = trimAscii(input);
        if (s.len >= 2 && s[0] == '(' && s[s.len - 1] == ')') {
            s = trimAscii(s[1 .. s.len - 1]);
        }
        if (s.len == 0) {
            return err.InvalidMediaQuery;
        }

        let name_raw: string = "";
        let val_raw: string = "";
        let op = CompareOp.eq;

        if (colon := std.mem.indexOfScalar(u8, s, ':')) {
            name_raw = trimAscii(s[0..colon]);
            val_raw = trimAscii(s[colon + 1 ..]);
            if (name_raw.len > 4 && std.ascii.eqlIgnoreCase(name_raw[0..4], "min-")) {
                op = CompareOp.ge;
                name_raw = name_raw[4..];
            } else if (name_raw.len > 4 && std.ascii.eqlIgnoreCase(name_raw[0..4], "max-")) {
                op = CompareOp.le;
                name_raw = name_raw[4..];
            }
        } else if (ge_idx := std.mem.indexOf(u8, s, ">=")) {
            name_raw = trimAscii(s[0..ge_idx]);
            val_raw = trimAscii(s[ge_idx + 2 ..]);
            op = CompareOp.ge;
        } else if (le_idx := std.mem.indexOf(u8, s, "<=")) {
            name_raw = trimAscii(s[0..le_idx]);
            val_raw = trimAscii(s[le_idx + 2 ..]);
            op = CompareOp.le;
        } else if (gt_idx := std.mem.indexOfScalar(u8, s, '>')) {
            name_raw = trimAscii(s[0..gt_idx]);
            val_raw = trimAscii(s[gt_idx + 1 ..]);
            op = CompareOp.gt;
        } else if (lt_idx := std.mem.indexOfScalar(u8, s, '<')) {
            name_raw = trimAscii(s[0..lt_idx]);
            val_raw = trimAscii(s[lt_idx + 1 ..]);
            op = CompareOp.lt;
        } else {
            return err.InvalidMediaQuery;
        }

        const fid = MediaFeatureId.parse(name_raw) ?? return err.InvalidMediaFeature;
        switch (fid) {
            .width, .height => {
                const px = parseLengthPx(val_raw)?;
                return MediaFeature{ feature: fid, op: op, px_value: px };
            },
            .orientation => {
                if (op != CompareOp.eq) { return err.InvalidFeatureValue; }
                const ori = MediaOrientation.parse(val_raw) ?? return err.InvalidFeatureValue;
                return MediaFeature{ feature: fid, orientation: ori };
            },
            .prefers_color_scheme => {
                if (op != CompareOp.eq) { return err.InvalidFeatureValue; }
                const cs = ColorScheme.parse(val_raw) ?? return err.InvalidFeatureValue;
                return MediaFeature{ feature: fid, color_scheme: cs };
            },
            .prefers_reduced_motion => {
                if (op != CompareOp.eq) { return err.InvalidFeatureValue; }
                const rm = ReducedMotion.parse(val_raw) ?? return err.InvalidFeatureValue;
                return MediaFeature{ feature: fid, reduced_motion: rm };
            },
            .hover => {
                if (op != CompareOp.eq) { return err.InvalidFeatureValue; }
                const hv = HoverCapability.parse(val_raw) ?? return err.InvalidFeatureValue;
                return MediaFeature{ feature: fid, hover: hv };
            },
            .pointer => {
                if (op != CompareOp.eq) { return err.InvalidFeatureValue; }
                const pt = PointerCapability.parse(val_raw) ?? return err.InvalidFeatureValue;
                return MediaFeature{ feature: fid, pointer: pt };
            },
        }
    }

    fn evaluate(env: MediaEnv): bool {
        return switch (_.feature) {
            .width => compareValues(env.width, _.px_value, _.op),
            .height => compareValues(env.height, _.px_value, _.op),
            .orientation => {
                const is_portrait = env.height >= env.width;
                return if (_.orientation == MediaOrientation.portrait) is_portrait else !is_portrait;
            },
            .prefers_color_scheme => env.color_scheme == _.color_scheme,
            .prefers_reduced_motion => env.reduced_motion == _.reduced_motion,
            .hover => env.hover == _.hover,
            .pointer => env.pointer == _.pointer,
        };
    }

    fn toCss(buf: []u8): usize ! CssMediaError {
        const fname = _.feature.toCss();
        const formatted = switch (_.feature) {
            .width, .height => switch (_.op) {
                .eq => std.fmt.bufPrint(buf, "({s}: {d}px)", .{ fname, _.px_value }),
                .ge => std.fmt.bufPrint(buf, "(min-{s}: {d}px)", .{ fname, _.px_value }),
                .le => std.fmt.bufPrint(buf, "(max-{s}: {d}px)", .{ fname, _.px_value }),
                .gt => std.fmt.bufPrint(buf, "({s} > {d}px)", .{ fname, _.px_value }),
                .lt => std.fmt.bufPrint(buf, "({s} < {d}px)", .{ fname, _.px_value }),
            },
            .orientation => std.fmt.bufPrint(buf, "(orientation: {s})", .{_.orientation.toCss()}),
            .prefers_color_scheme => std.fmt.bufPrint(buf, "(prefers-color-scheme: {s})", .{_.color_scheme.toCss()}),
            .prefers_reduced_motion => std.fmt.bufPrint(buf, "(prefers-reduced-motion: {s})", .{_.reduced_motion.toCss()}),
            .hover => std.fmt.bufPrint(buf, "(hover: {s})", .{_.hover.toCss()}),
            .pointer => std.fmt.bufPrint(buf, "(pointer: {s})", .{_.pointer.toCss()}),
        } catch return err.BufferTooSmall;
        return formatted.len;
    }
}

/// 单条媒体查询（可选限定符 + 可选媒体类型 + 最多 8 项 `and` 连接的特征条件，零堆分配）
pub struct MediaQuery {
    qualifier: MediaQualifier = MediaQualifier.none,
    media_type: MediaType = MediaType.all,
    explicit_type: bool = false,
    features: [8]MediaFeature = [
        MediaFeature{ feature: MediaFeatureId.width },
        MediaFeature{ feature: MediaFeatureId.width },
        MediaFeature{ feature: MediaFeatureId.width },
        MediaFeature{ feature: MediaFeatureId.width },
        MediaFeature{ feature: MediaFeatureId.width },
        MediaFeature{ feature: MediaFeatureId.width },
        MediaFeature{ feature: MediaFeatureId.width },
        MediaFeature{ feature: MediaFeatureId.width },
    ],
    feature_count: usize = 0,

    static fn parse(input: string): MediaQuery ! CssMediaError {
        let s = trimAscii(input);
        if (s.len == 0) {
            return err.InvalidMediaQuery;
        }
        let q = MediaQuery{};

        // 1. 若不以 '(' 开头，解析可选限定符（only/not）与媒体类型（all/screen/print）
        if (s[0] != '(') {
            let tok_end: usize = 0;
            while (tok_end < s.len && s[tok_end] != ' ' && s[tok_end] != '\t' && s[tok_end] != '(') {
                tok_end += 1;
            }
            const first_tok = s[0..tok_end];
            if (qual := MediaQualifier.parse(first_tok)) {
                q.qualifier = qual;
                s = trimAscii(s[tok_end..]);
                if (s.len == 0) {
                    return err.InvalidMediaQuery;
                }
            }
            if (s[0] != '(') {
                let mt_end: usize = 0;
                while (mt_end < s.len && s[mt_end] != ' ' && s[mt_end] != '\t' && s[mt_end] != '(') {
                    mt_end += 1;
                }
                if (mt := MediaType.parse(s[0..mt_end])) {
                    q.media_type = mt;
                } else {
                    return err.InvalidMediaType;
                }
                q.explicit_type = true;
                s = trimAscii(s[mt_end..]);

                if (s.len == 0) {
                    return q;
                }
                // 媒体类型之后必须紧跟 `and `
                if (s.len < 4 || !std.ascii.eqlIgnoreCase(s[0..3], "and") || (s[3] != ' ' && s[3] != '\t')) {
                    return err.InvalidMediaQuery;
                }
                s = trimAscii(s[3..]);
                if (s.len == 0 || s[0] != '(') {
                    return err.InvalidMediaQuery;
                }
            }
        }

        // 2. 解析 `(...)` 与 `and` 连接的特征序列
        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) {
                pos += 1;
            }
            if (pos >= s.len) {
                break;
            }
            if (s[pos] != '(') {
                return err.InvalidMediaQuery;
            }
            const close = std.mem.indexOfScalarPos(u8, s, pos + 1, ')') ?? return err.InvalidMediaQuery;
            const feat = MediaFeature.parse(s[pos .. close + 1])?;
            if (q.feature_count >= 8) {
                return err.TooManyFeatures;
            }
            q.features[q.feature_count] = feat;
            q.feature_count += 1;
            pos = close + 1;

            while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) {
                pos += 1;
            }
            if (pos >= s.len) {
                break;
            }
            if (pos + 3 > s.len || !std.ascii.eqlIgnoreCase(s[pos .. pos + 3], "and")) {
                return err.InvalidMediaQuery;
            }
            pos += 3;
            const tail = trimAscii(s[pos..]);
            if (tail.len == 0 || tail[0] != '(') {
                return err.InvalidMediaQuery;
            }
        }

        if (!q.explicit_type && q.feature_count == 0) {
            return err.InvalidMediaQuery;
        }
        return q;
    }

    /// 是否恒真（等价于无条件的 `all`）
    fn alwaysMatches(): bool {
        return _.qualifier != MediaQualifier.not_kw
            && _.media_type == MediaType.all
            && _.feature_count == 0;
    }

    /// 是否恒假（等价于 `not all`）
    fn neverMatches(): bool {
        return _.qualifier == MediaQualifier.not_kw
            && _.media_type == MediaType.all
            && _.feature_count == 0;
    }

    fn evaluate(env: MediaEnv): bool {
        let matched = (_.media_type == MediaType.all) || (_.media_type == env.media_type);
        let i: usize = 0;
        while (matched && i < _.feature_count) {
            if (!_.features[i].evaluate(env)) {
                matched = false;
            }
            i += 1;
        }
        return if (_.qualifier == MediaQualifier.not_kw) !matched else matched;
    }

    fn toCss(buf: []u8): usize ! CssMediaError {
        let pos: usize = 0;
        if (_.qualifier != MediaQualifier.none) {
            const qstr = _.qualifier.toCss();
            if (pos + qstr.len + 1 > buf.len) { return err.BufferTooSmall; }
            @memcpy(buf[pos .. pos + qstr.len], qstr);
            pos += qstr.len;
            buf[pos] = ' ';
            pos += 1;
        }
        if (_.explicit_type || _.feature_count == 0) {
            const mstr = _.media_type.toCss();
            if (pos + mstr.len > buf.len) { return err.BufferTooSmall; }
            @memcpy(buf[pos .. pos + mstr.len], mstr);
            pos += mstr.len;
            if (_.feature_count > 0) {
                const sep = " and ";
                if (pos + sep.len > buf.len) { return err.BufferTooSmall; }
                @memcpy(buf[pos .. pos + sep.len], sep);
                pos += sep.len;
            }
        }
        let i: usize = 0;
        while (i < _.feature_count) {
            if (i > 0) {
                const sep = " and ";
                if (pos + sep.len > buf.len) { return err.BufferTooSmall; }
                @memcpy(buf[pos .. pos + sep.len], sep);
                pos += sep.len;
            }
            const written = _.features[i].toCss(buf[pos..])?;
            pos += written;
            i += 1;
        }
        return pos;
    }
}

/// 逗号分隔的媒体查询列表（逻辑 OR，最多 8 条查询，零堆分配）
pub struct MediaQueryList {
    queries: [8]MediaQuery = [
        MediaQuery{},
        MediaQuery{},
        MediaQuery{},
        MediaQuery{},
        MediaQuery{},
        MediaQuery{},
        MediaQuery{},
        MediaQuery{},
    ],
    len: usize = 0,

    static fn parse(input: string): MediaQueryList ! CssMediaError {
        let s = trimAscii(input);
        if (s.len > 6 && std.ascii.eqlIgnoreCase(s[0..6], "@media")) {
            s = trimAscii(s[6..]);
        }
        if (s.len == 0) {
            return err.InvalidMediaQuery;
        }

        let list = MediaQueryList{};
        let start: usize = 0;
        let pos: usize = 0;
        while (pos <= s.len) {
            if (pos == s.len || s[pos] == ',') {
                const part = trimAscii(s[start..pos]);
                if (part.len == 0) {
                    return err.InvalidMediaQuery;
                }
                if (list.len >= 8) {
                    return err.TooManyQueries;
                }
                list.queries[list.len] = MediaQuery.parse(part)?;
                list.len += 1;
                start = pos + 1;
            }
            pos += 1;
        }
        return list;
    }

    fn evaluate(env: MediaEnv): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.queries[i].evaluate(env)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn alwaysMatches(): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.queries[i].alwaysMatches()) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn neverMatches(): bool {
        if (_.len == 0) {
            return true;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!_.queries[i].neverMatches()) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    fn toCss(buf: []u8): usize ! CssMediaError {
        const prefix = "@media ";
        if (prefix.len > buf.len) { return err.BufferTooSmall; }
        @memcpy(buf[0..prefix.len], prefix);
        let pos: usize = prefix.len;

        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                const sep = ", ";
                if (pos + sep.len > buf.len) { return err.BufferTooSmall; }
                @memcpy(buf[pos .. pos + sep.len], sep);
                pos += sep.len;
            }
            const written = _.queries[i].toCss(buf[pos..])?;
            pos += written;
            i += 1;
        }
        return pos;
    }
}
