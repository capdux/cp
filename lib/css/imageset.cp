// Source: Absorbed from /data/dev/bun/src/css/values/image.cp | Synced: 2026-11-22T05:10:12Z

/// CSS `<resolution>` 单位枚举
pub enum ResolutionUnit: u8 {
    dppx = 0,
    x = 1,
    dpi = 2,
    dpcm = 3,

    fn asStr(): string {
        return switch (_) {
            .dppx => "dppx",
            .x => "x",
            .dpi => "dpi",
            .dpcm => "dpcm",
        };
    }
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) { start += 1; }
    while (end > start && isSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCaseAscii(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) { return false; }
    return eqlIgnoreCaseAscii(s[0..prefix.len], prefix);
}

struct BufWriter {
    buf: []u8,
    pos: usize = 0,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        }
    }

    fn writeStr(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len && _.pos < _.buf.len) {
            _.buf[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
    }

    fn writeFloat(*_, val: f32) {
        let v: f32 = val;
        if (v < 0.0) {
            _.writeByte('-');
            v = 0.0 - v;
        }
        const scaled: u64 = @intFromFloat(@round(@as(f64, v) * 10000.0));
        const int_part: u64 = scaled / 10000;
        let frac_part: u64 = scaled % 10000;

        let digits: [24]u8 = [0; 24];
        let dlen: usize = 0;
        let tmp = int_part;
        if (tmp == 0) {
            digits[0] = '0';
            dlen = 1;
        } else {
            while (tmp > 0 && dlen < 24) {
                digits[dlen] = @intCast((tmp % 10) + '0');
                tmp /= 10;
                dlen += 1;
            }
        }
        let k: usize = dlen;
        while (k > 0) {
            k -= 1;
            _.writeByte(digits[k]);
        }

        if (frac_part > 0) {
            let fbuf: [4]u8 = [
                @intCast(((frac_part / 1000) % 10) + '0'),
                @intCast(((frac_part / 100) % 10) + '0'),
                @intCast(((frac_part / 10) % 10) + '0'),
                @intCast((frac_part % 10) + '0'),
            ];
            let flen: usize = 4;
            while (flen > 0 && fbuf[flen - 1] == '0') {
                flen -= 1;
            }
            _.writeByte('.');
            let fi: usize = 0;
            while (fi < flen) {
                _.writeByte(fbuf[fi]);
                fi += 1;
            }
        }
    }

    fn slice(): []const u8 {
        return _.buf[0.._.pos];
    }
}

/// CSS `<resolution>` 值（支持 `dppx` / `x` / `dpi` / `dpcm`）
pub struct Resolution {
    value: f32 = 1.0,
    unit: ResolutionUnit = .x,

    /// 解析 `<resolution>`，拒绝负数或未知单位
    static fn parse(input: []const u8): Resolution? {
        const s = trimSpaces(input);
        if (s.len == 0) { return null; }

        let i: usize = 0;
        if (s[i] == '-') {
            return null;
        }
        if (s[i] == '+') {
            i += 1;
        }

        let has_digits = false;
        let int_val: f64 = 0.0;
        while (i < s.len && isDigit(s[i])) {
            has_digits = true;
            int_val = int_val * 10.0 + @as(f64, @floatFromInt(s[i] - '0'));
            i += 1;
        }

        let frac_val: f64 = 0.0;
        let frac_scale: f64 = 0.1;
        if (i < s.len && s[i] == '.') {
            i += 1;
            while (i < s.len && isDigit(s[i])) {
                has_digits = true;
                frac_val += @as(f64, @floatFromInt(s[i] - '0')) * frac_scale;
                frac_scale *= 0.1;
                i += 1;
            }
        }

        if (!has_digits || i >= s.len) {
            return null;
        }

        const unit_str = s[i..s.len];
        let unit: ResolutionUnit = .x;
        if (eqlIgnoreCaseAscii(unit_str, "dppx")) {
            unit = .dppx;
        } else if (eqlIgnoreCaseAscii(unit_str, "x")) {
            unit = .x;
        } else if (eqlIgnoreCaseAscii(unit_str, "dpi")) {
            unit = .dpi;
        } else if (eqlIgnoreCaseAscii(unit_str, "dpcm")) {
            unit = .dpcm;
        } else {
            return null;
        }

        return Resolution{
            value: @floatCast(int_val + frac_val),
            unit: unit,
        };
    }

    /// 换算为标准 `dppx`（`96dpi == 1dppx`，`1dpcm == 2.54/96 dppx`）
    fn toDppx(): f32 {
        return switch (_.unit) {
            .dppx => _.value,
            .x => _.value,
            .dpi => _.value / 96.0,
            .dpcm => (_.value * 2.54) / 96.0,
        };
    }

    /// 规范化分辨率（`dppx` 与 `x` 统一规范化为 `x`）
    fn normalized(): Resolution {
        if (_.unit == .dppx) {
            return Resolution{ value: _.value, unit: .x };
        }
        return _;
    }

    fn writeCss(w: *BufWriter) {
        w.writeFloat(_.value);
        switch (_.unit) {
            .dppx => w.writeStr("x"),
            .x => w.writeStr("x"),
            .dpi => w.writeStr("dpi"),
            .dpcm => w.writeStr("dpcm"),
        }
    }

    /// 规范化序列化为 CSS 字符串（`dppx` 规范化输出为 `x`）
    fn toCss(out: []u8): []const u8 {
        let w = BufWriter{ buf: out };
        _.writeCss(*w);
        return w.slice();
    }
}

struct QuotedResult {
    text: []const u8,
    next: usize,
}

fn parseQuotedStringAt(s: []const u8, start: usize): QuotedResult? {
    if (start >= s.len) { return null; }
    const q = s[start];
    if (q != '"' && q != '\'') { return null; }
    let i = start + 1;
    const content_start = i;
    while (i < s.len) {
        if (s[i] == '\\' && i + 1 < s.len) {
            i += 2;
            continue;
        }
        if (s[i] == q) {
            return QuotedResult{
                text: s[content_start..i],
                next: i + 1,
            };
        }
        i += 1;
    }
    return null;
}

fn parseUrlFuncAt(s: []const u8, start: usize): QuotedResult? {
    if (!startsWithIgnoreCaseAscii(s[start..s.len], "url(")) {
        return null;
    }
    let i = start + 4;
    while (i < s.len && isSpace(s[i])) { i += 1; }
    if (i >= s.len) { return null; }

    let url_text: []const u8 = "";
    if (s[i] == '"' || s[i] == '\'') {
        if (q := parseQuotedStringAt(s, i)) {
            url_text = q.text;
            i = q.next;
        } else {
            return null;
        }
        while (i < s.len && isSpace(s[i])) { i += 1; }
        if (i >= s.len || s[i] != ')') { return null; }
        return QuotedResult{ text: url_text, next: i + 1 };
    }

    const raw_start = i;
    while (i < s.len && s[i] != ')') {
        if (s[i] == '"' || s[i] == '\'' || s[i] == '(') {
            return null;
        }
        i += 1;
    }
    if (i >= s.len || s[i] != ')') { return null; }
    url_text = trimSpaces(s[raw_start..i]);
    if (url_text.len == 0) { return null; }
    return QuotedResult{ text: url_text, next: i + 1 };
}

fn parseTypeFuncAt(s: []const u8, start: usize): QuotedResult? {
    if (!startsWithIgnoreCaseAscii(s[start..s.len], "type(")) {
        return null;
    }
    let i = start + 5;
    while (i < s.len && isSpace(s[i])) { i += 1; }
    if (q := parseQuotedStringAt(s, i)) {
        i = q.next;
        while (i < s.len && isSpace(s[i])) { i += 1; }
        if (i >= s.len || s[i] != ')') { return null; }
        if (q.text.len == 0) { return null; }
        return QuotedResult{ text: q.text, next: i + 1 };
    }
    return null;
}

/// `image-set()` 单条候选项（URL + 分辨率 + 可选 MIME 类型）
pub struct ImageSetOption {
    url: []const u8 = "",
    resolution: Resolution = Resolution{ value: 1.0, unit: .x },
    mime_type: ([]const u8)? = null,

    /// 解析单条 `image-set()` 候选项（支持 `url(...)` 或裸字符串，分辨率与 `type(...)` 顺序不限）
    static fn parse(input: []const u8): ImageSetOption? {
        const s = trimSpaces(input);
        if (s.len == 0) { return null; }

        let i: usize = 0;
        let url_val: []const u8 = "";
        if (u := parseUrlFuncAt(s, i)) {
            url_val = u.text;
            i = u.next;
        } else if (q := parseQuotedStringAt(s, i)) {
            url_val = q.text;
            i = q.next;
        } else {
            return null;
        }
        if (url_val.len == 0) { return null; }

        let res = Resolution{ value: 1.0, unit: .x };
        let has_res = false;
        let mime: ([]const u8)? = null;

        while (i < s.len) {
            while (i < s.len && isSpace(s[i])) { i += 1; }
            if (i >= s.len) { break; }

            if (startsWithIgnoreCaseAscii(s[i..s.len], "type(")) {
                if (mime != null) { return null; }
                if (t := parseTypeFuncAt(s, i)) {
                    mime = t.text;
                    i = t.next;
                } else {
                    return null;
                }
            } else {
                if (has_res) { return null; }
                const tok_start = i;
                while (i < s.len && !isSpace(s[i])) { i += 1; }
                if (parsed_res := Resolution.parse(s[tok_start..i])) {
                    if (parsed_res.value <= 0.0) { return null; }
                    res = parsed_res;
                    has_res = true;
                } else {
                    return null;
                }
            }
        }

        return ImageSetOption{
            url: url_val,
            resolution: res,
            mime_type: mime,
        };
    }

    fn writeCss(w: *BufWriter, is_prefixed: bool) {
        if (is_prefixed) {
            w.writeStr("url(\"");
            w.writeStr(_.url);
            w.writeStr("\") ");
        } else {
            w.writeByte('"');
            w.writeStr(_.url);
            w.writeStr("\" ");
        }
        _.resolution.writeCss(w);
        if (m := _.mime_type) {
            w.writeStr(" type(\"");
            w.writeStr(m);
            w.writeStr("\")");
        }
    }

    fn toCss(out: []u8, is_prefixed: bool): []const u8 {
        let w = BufWriter{ buf: out };
        _.writeCss(*w, is_prefixed);
        return w.slice();
    }
}

/// MIME 支持检测回调类型
pub type MimeSupportFn = fn ([]const u8): bool;

/// CSS `image-set()` / `-webkit-image-set()` 零堆分配值模型（最多 16 项候选）
pub struct ImageSet {
    options: [16]ImageSetOption = [ImageSetOption{}; 16],
    len: usize = 0,
    is_webkit_prefixed: bool = false,

    /// 解析 `image-set(...)` 或 `-webkit-image-set(...)`
    static fn parse(input: []const u8): ImageSet? {
        const s = trimSpaces(input);
        let is_webkit = false;
        let inner_start: usize = 0;

        if (startsWithIgnoreCaseAscii(s, "image-set(")) {
            inner_start = 10;
        } else if (startsWithIgnoreCaseAscii(s, "-webkit-image-set(")) {
            is_webkit = true;
            inner_start = 18;
        } else {
            return null;
        }

        if (s.len <= inner_start || s[s.len - 1] != ')') {
            return null;
        }
        const inner = s[inner_start..(s.len - 1)];

        let result = ImageSet{
            is_webkit_prefixed: is_webkit,
        };

        let i: usize = 0;
        let seg_start: usize = 0;
        let paren_depth: usize = 0;
        let in_quote: u8 = 0;

        while (i <= inner.len) {
            const at_end = (i == inner.len);
            let split_here = at_end;

            if (!at_end) {
                const c = inner[i];
                if (in_quote != 0) {
                    if (c == '\\' && i + 1 < inner.len) {
                        i += 2;
                        continue;
                    }
                    if (c == in_quote) {
                        in_quote = 0;
                    }
                } else {
                    if (c == '"' || c == '\'') {
                        in_quote = c;
                    } else if (c == '(') {
                        paren_depth += 1;
                    } else if (c == ')') {
                        if (paren_depth == 0) { return null; }
                        paren_depth -= 1;
                    } else if (c == ',' && paren_depth == 0) {
                        split_here = true;
                    }
                }
            }

            if (split_here) {
                if (in_quote != 0 || paren_depth != 0) {
                    return null;
                }
                const seg = trimSpaces(inner[seg_start..i]);
                if (seg.len == 0 || result.len >= 16) {
                    return null;
                }
                if (opt := ImageSetOption.parse(seg)) {
                    result.options[result.len] = opt;
                    result.len += 1;
                } else {
                    return null;
                }
                seg_start = i + 1;
            }
            i += 1;
        }

        if (result.len == 0) {
            return null;
        }
        return result;
    }

    /// 按设备像素比 `dpr` 与 MIME 支持回调选择最优图片候选项
    /// 优先选择满足 `toDppx() >= dpr` 的最小分辨率；若均低于 `dpr` 则回退至最大可用分辨率
    fn selectBest(dpr: f32, supported_mime_fn: MimeSupportFn?): ImageSetOption? {
        let best_ge_idx: usize? = null;
        let best_ge_dppx: f32 = 0.0;
        let best_lt_idx: usize? = null;
        let best_lt_dppx: f32 = 0.0;

        let i: usize = 0;
        while (i < _.len) {
            const opt = _.options[i];
            let supported = true;
            if (m := opt.mime_type) {
                if (cb := supported_mime_fn) {
                    supported = cb(m);
                }
            }
            if (supported) {
                const dppx = opt.resolution.toDppx();
                if (dppx >= dpr) {
                    if (best_ge_idx == null || dppx < best_ge_dppx) {
                        best_ge_idx = i;
                        best_ge_dppx = dppx;
                    }
                } else {
                    if (best_lt_idx == null || dppx > best_lt_dppx) {
                        best_lt_idx = i;
                        best_lt_dppx = dppx;
                    }
                }
            }
            i += 1;
        }

        if (idx := best_ge_idx) {
            return _.options[idx];
        }
        if (idx := best_lt_idx) {
            return _.options[idx];
        }
        return null;
    }

    fn writeCss(w: *BufWriter) {
        if (_.is_webkit_prefixed) {
            w.writeStr("-webkit-image-set(");
        } else {
            w.writeStr("image-set(");
        }
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                w.writeStr(", ");
            }
            _.options[i].writeCss(w, _.is_webkit_prefixed);
            i += 1;
        }
        w.writeByte(')');
    }

    /// 规范化序列化为 CSS `image-set(...)` 或 `-webkit-image-set(...)`
    fn toCss(out: []u8): []const u8 {
        let w = BufWriter{ buf: out };
        _.writeCss(*w);
        return w.slice();
    }
}

/// CSS `<image>` 统一分类枚举
pub enum CssImageKind: u8 {
    none = 0,
    url = 1,
    image_set = 2,
    gradient_raw = 3,

    fn asStr(): string {
        return switch (_) {
            .none => "none",
            .url => "url",
            .image_set => "image-set",
            .gradient_raw => "gradient",
        };
    }
}

fn isBalancedGradientCall(s: []const u8): bool {
    const is_grad =
        startsWithIgnoreCaseAscii(s, "linear-gradient(") ||
        startsWithIgnoreCaseAscii(s, "repeating-linear-gradient(") ||
        startsWithIgnoreCaseAscii(s, "radial-gradient(") ||
        startsWithIgnoreCaseAscii(s, "repeating-radial-gradient(") ||
        startsWithIgnoreCaseAscii(s, "conic-gradient(") ||
        startsWithIgnoreCaseAscii(s, "repeating-conic-gradient(") ||
        startsWithIgnoreCaseAscii(s, "-webkit-linear-gradient(") ||
        startsWithIgnoreCaseAscii(s, "-webkit-radial-gradient(") ||
        startsWithIgnoreCaseAscii(s, "-webkit-gradient(");
    if (!is_grad || s[s.len - 1] != ')') {
        return false;
    }
    let depth: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '(') {
            depth += 1;
        } else if (s[i] == ')') {
            if (depth == 0) { return false; }
            depth -= 1;
            if (depth == 0 && i + 1 < s.len) {
                return false;
            }
        }
        i += 1;
    }
    return depth == 0;
}

/// CSS Images 4 `<image>` 统一包装模型（`none` / `url()` / `image-set()` / 渐变原始值）
pub struct CssImage {
    kind: CssImageKind = .none,
    url: []const u8 = "",
    image_set: ImageSet = ImageSet{},
    gradient_raw: []const u8 = "",

    /// 解析任意 CSS `<image>` 值
    static fn parse(input: []const u8): CssImage? {
        const s = trimSpaces(input);
        if (s.len == 0) { return null; }

        if (eqlIgnoreCaseAscii(s, "none")) {
            return CssImage{ kind: .none };
        }

        if (u := parseUrlFuncAt(s, 0)) {
            if (trimSpaces(s[u.next..s.len]).len == 0) {
                return CssImage{
                    kind: .url,
                    url: u.text,
                };
            }
            return null;
        }

        if (startsWithIgnoreCaseAscii(s, "image-set(") ||
            startsWithIgnoreCaseAscii(s, "-webkit-image-set("))
        {
            if (iset := ImageSet.parse(s)) {
                return CssImage{
                    kind: .image_set,
                    image_set: iset,
                };
            }
            return null;
        }

        if (isBalancedGradientCall(s)) {
            return CssImage{
                kind: .gradient_raw,
                gradient_raw: s,
            };
        }

        return null;
    }

    /// 序列化为规范 CSS `<image>` 字符串
    fn toCss(out: []u8): []const u8 {
        let w = BufWriter{ buf: out };
        switch (_.kind) {
            .none => w.writeStr("none"),
            .url => {
                w.writeStr("url(\"");
                w.writeStr(_.url);
                w.writeStr("\")");
            },
            .image_set => _.image_set.writeCss(*w),
            .gradient_raw => w.writeStr(_.gradient_raw),
        }
        return w.slice();
    }
}
