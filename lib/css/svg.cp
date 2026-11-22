// Source: Absorbed from /data/dev/bun/src/css/properties/svg.cp | Synced: 2026-11-22T12:49:17Z

pub const MAX_DASH_ENTRIES: usize = 16;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn trimSpaces(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) { start += 1; }
    while (end > start && isSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: string, b: string): bool {
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

fn parseNonNegFloat(s: string): f64? {
    const t = trimSpaces(s);
    if (t.len == 0) { return null; }
    const v = std.fmt.parseFloat(f64, t) catch return null;
    if (!std.math.isFinite(v) || v < 0.0) { return null; }
    return v;
}

fn writeU64(v_in: u64, dst: []u8): usize {
    if (v_in == 0) {
        dst[0] = '0';
        return 1;
    }
    let rev: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = v_in;
    while (v > 0) {
        rev[n] = ('0' as u8) + ((v % 10) as u8);
        v /= 10;
        n += 1;
    }
    let i: usize = 0;
    while (i < n) {
        dst[i] = rev[n - 1 - i];
        i += 1;
    }
    return n;
}

fn appendStr(buf: []u8, pos: *usize, s: string): bool {
    if (pos.* + s.len > buf.len) { return false; }
    if (s.len > 0) {
        @memcpy(buf[pos.* .. pos.* + s.len], s);
        pos.* += s.len;
    }
    return true;
}

fn appendNum(buf: []u8, pos: *usize, val_in: f64): bool {
    if (@abs(val_in) <= 0.0000005) {
        return appendStr(buf, pos, "0");
    }
    let tmp: [64]u8 = [0; 64];
    let p: usize = 0;
    const scaled = @round(val_in * 1000000.0);
    const total_u64 = scaled as u64;
    const int_part = total_u64 / 1000000;
    const frac_part = total_u64 % 1000000;
    p += writeU64(int_part, tmp[p..64]);
    if (frac_part > 0) {
        tmp[p] = '.';
        p += 1;
        let fbuf: [6]u8 = ['0'; 6];
        let f = frac_part;
        let k: usize = 6;
        while (k > 0) {
            k -= 1;
            fbuf[k] = ('0' as u8) + ((f % 10) as u8);
            f /= 10;
        }
        let fend: usize = 6;
        while (fend > 0 && fbuf[fend - 1] == '0') { fend -= 1; }
        @memcpy(tmp[p .. p + fend], fbuf[0..fend]);
        p += fend;
    }
    return appendStr(buf, pos, tmp[0..p]);
}

fn splitBalancedSpace(s: string, out: []string): usize? {
    let count: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        while (i < s.len && isSpace(s[i])) { i += 1; }
        if (i >= s.len) { break; }
        const start = i;
        let depth: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (c == '(') {
                depth += 1;
            } else if (c == ')') {
                if (depth == 0) { return null; }
                depth -= 1;
            } else if (depth == 0 && isSpace(c)) {
                break;
            }
            i += 1;
        }
        if (depth != 0) { return null; }
        if (count >= out.len) { return null; }
        out[count] = s[start..i];
        count += 1;
    }
    return count;
}

/// SVG `<paint>` 类型枚举
pub enum SVGPaintKind: u8 {
    none = 0,
    color = 1,
    url = 2,
    context_fill = 3,
    context_stroke = 4,
}

/// `url(...)` 绘制服务器回退类型
pub enum SVGPaintFallbackKind: u8 {
    absent = 0,
    none = 1,
    color = 2,
}

/// SVG `<paint>` 值（用于 `fill` / `stroke`，支持 `url(#id) fallback` 与 `context-fill`/`context-stroke`）
pub struct SVGPaint {
    kind: SVGPaintKind = .none,
    url_ref: string = "",
    color_val: string = "",
    fallback_kind: SVGPaintFallbackKind = .absent,
    fallback_color: string = "",

    static fn parse(input: string): SVGPaint? {
        const s = trimSpaces(input);
        if (s.len == 0) { return null; }

        let toks: [4]string = ["", "", "", ""];
        const n = splitBalancedSpace(s, toks[0..4]) ?? return null;
        if (n == 0 || n > 2) { return null; }

        const first = toks[0];

        // url(...) 形式（可带可选回退：none 或颜色）
        if (first.len >= 5 && eqlIgnoreCaseAscii(first[0..4], "url(") && first[first.len - 1] == ')') {
            let inner = trimSpaces(first[4 .. first.len - 1]);
            if (inner.len >= 2 && ((inner[0] == '"' && inner[inner.len - 1] == '"') || (inner[0] == '\'' && inner[inner.len - 1] == '\''))) {
                inner = inner[1 .. inner.len - 1];
            }
            if (inner.len == 0) { return null; }

            let fb_kind: SVGPaintFallbackKind = .absent;
            let fb_color: string = "";
            if (n == 2) {
                const second = toks[1];
                if (eqlIgnoreCaseAscii(second, "none")) {
                    fb_kind = .none;
                } else {
                    fb_kind = .color;
                    fb_color = second;
                }
            }
            return SVGPaint{
                kind: .url,
                url_ref: inner,
                fallback_kind: fb_kind,
                fallback_color: fb_color,
            };
        }

        if (n != 1) { return null; }
        if (eqlIgnoreCaseAscii(first, "none")) {
            return SVGPaint{ kind: .none };
        }
        if (eqlIgnoreCaseAscii(first, "context-fill")) {
            return SVGPaint{ kind: .context_fill };
        }
        if (eqlIgnoreCaseAscii(first, "context-stroke")) {
            return SVGPaint{ kind: .context_stroke };
        }
        return SVGPaint{ kind: .color, color_val: first };
    }

    fn toCss(out: []u8): []const u8 {
        let pos: usize = 0;
        switch (_.kind) {
            .none => {
                if (!appendStr(out, *pos, "none")) { return out[0..0]; }
            },
            .context_fill => {
                if (!appendStr(out, *pos, "context-fill")) { return out[0..0]; }
            },
            .context_stroke => {
                if (!appendStr(out, *pos, "context-stroke")) { return out[0..0]; }
            },
            .color => {
                if (!appendStr(out, *pos, _.color_val)) { return out[0..0]; }
            },
            .url => {
                if (!appendStr(out, *pos, "url(")) { return out[0..0]; }
                if (!appendStr(out, *pos, _.url_ref)) { return out[0..0]; }
                if (!appendStr(out, *pos, ")")) { return out[0..0]; }
                if (_.fallback_kind == .none) {
                    if (!appendStr(out, *pos, " none")) { return out[0..0]; }
                } else if (_.fallback_kind == .color) {
                    if (!appendStr(out, *pos, " ")) { return out[0..0]; }
                    if (!appendStr(out, *pos, _.fallback_color)) { return out[0..0]; }
                }
            },
        }
        return out[0..pos];
    }
}

/// SVG `stroke-linecap` 枚举
pub enum StrokeLinecap: u8 {
    butt = 0,
    round = 1,
    square = 2,

    static fn fromStr(s: string): StrokeLinecap? {
        const t = trimSpaces(s);
        if (eqlIgnoreCaseAscii(t, "butt")) { return .butt; }
        if (eqlIgnoreCaseAscii(t, "round")) { return .round; }
        if (eqlIgnoreCaseAscii(t, "square")) { return .square; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .butt => "butt",
            .round => "round",
            .square => "square",
        };
    }
}

/// SVG `stroke-linejoin` 枚举
pub enum StrokeLinejoin: u8 {
    miter = 0,
    round = 1,
    bevel = 2,
    miter_clip = 3,
    arcs = 4,

    static fn fromStr(s: string): StrokeLinejoin? {
        const t = trimSpaces(s);
        if (eqlIgnoreCaseAscii(t, "miter")) { return .miter; }
        if (eqlIgnoreCaseAscii(t, "round")) { return .round; }
        if (eqlIgnoreCaseAscii(t, "bevel")) { return .bevel; }
        if (eqlIgnoreCaseAscii(t, "miter-clip")) { return .miter_clip; }
        if (eqlIgnoreCaseAscii(t, "arcs")) { return .arcs; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .miter => "miter",
            .round => "round",
            .bevel => "bevel",
            .miter_clip => "miter-clip",
            .arcs => "arcs",
        };
    }
}

/// `stroke-dasharray` 单项单位
pub enum DashUnit: u8 {
    number = 0,
    px = 1,
    percent = 2,
}

/// `stroke-dasharray` 单项
pub struct DashEntry {
    value: f64 = 0.0,
    unit: DashUnit = .number,

    static fn parse(tok: string): DashEntry? {
        const s = trimSpaces(tok);
        if (s.len == 0) { return null; }
        if (s[s.len - 1] == '%') {
            const v = parseNonNegFloat(s[0 .. s.len - 1]) ?? return null;
            return DashEntry{ value: v, unit: .percent };
        }
        if (s.len >= 2 && eqlIgnoreCaseAscii(s[s.len - 2 ..], "px")) {
            const v = parseNonNegFloat(s[0 .. s.len - 2]) ?? return null;
            return DashEntry{ value: v, unit: .px };
        }
        const v = parseNonNegFloat(s) ?? return null;
        return DashEntry{ value: v, unit: .number };
    }

    fn writeTo(buf: []u8, pos: *usize): bool {
        if (!appendNum(buf, pos, _.value)) { return false; }
        return switch (_.unit) {
            .number => true,
            .px => appendStr(buf, pos, "px"),
            .percent => appendStr(buf, pos, "%"),
        };
    }
}

/// SVG `stroke-dasharray`（`none` 或最多 16 项非负长度/百分比/无单位数，逗号或空白分隔）
pub struct StrokeDasharray {
    is_none: bool = true,
    items: [MAX_DASH_ENTRIES]DashEntry = [DashEntry{}; MAX_DASH_ENTRIES],
    len: usize = 0,

    static fn none(): StrokeDasharray {
        return StrokeDasharray{};
    }

    static fn parse(input: string): StrokeDasharray? {
        const s = trimSpaces(input);
        if (s.len == 0) { return null; }
        if (eqlIgnoreCaseAscii(s, "none")) {
            return StrokeDasharray.none();
        }

        let res = StrokeDasharray{ is_none: false };
        let i: usize = 0;
        let expect_value = true;

        while (i < s.len) {
            while (i < s.len && isSpace(s[i])) { i += 1; }
            if (i >= s.len) { break; }

            if (s[i] == ',') {
                if (expect_value) { return null; }
                expect_value = true;
                i += 1;
                continue;
            }

            const start = i;
            while (i < s.len && !isSpace(s[i]) && s[i] != ',') { i += 1; }
            const tok = s[start..i];
            const entry = DashEntry.parse(tok) ?? return null;
            if (res.len >= MAX_DASH_ENTRIES) { return null; }
            res.items[res.len] = entry;
            res.len += 1;
            expect_value = false;
        }

        if (expect_value || res.len == 0) {
            return null;
        }
        return res;
    }

    fn slice(): []const DashEntry {
        if (_.is_none) {
            return _.items[0..0];
        }
        return _.items[0.._.len];
    }

    fn toCss(out: []u8): []const u8 {
        let pos: usize = 0;
        if (_.is_none || _.len == 0) {
            if (!appendStr(out, *pos, "none")) { return out[0..0]; }
            return out[0..pos];
        }
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                if (!appendStr(out, *pos, " ")) { return out[0..0]; }
            }
            if (!_.items[i].writeTo(out, *pos)) { return out[0..0]; }
            i += 1;
        }
        return out[0..pos];
    }
}

/// SVG `paint-order` 单项枚举
pub enum PaintOrderItem: u8 {
    fill = 0,
    stroke = 1,
    markers = 2,

    static fn fromStr(s: string): PaintOrderItem? {
        const t = trimSpaces(s);
        if (eqlIgnoreCaseAscii(t, "fill")) { return .fill; }
        if (eqlIgnoreCaseAscii(t, "stroke")) { return .stroke; }
        if (eqlIgnoreCaseAscii(t, "markers")) { return .markers; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .fill => "fill",
            .stroke => "stroke",
            .markers => "markers",
        };
    }
}

/// SVG `paint-order`（`normal` 或 1~3 项无重复排列，自动补全并按最短形式规范序列化）
pub struct PaintOrder {
    order: [3]PaintOrderItem = [.fill, .stroke, .markers],

    static fn normal(): PaintOrder {
        return PaintOrder{};
    }

    static fn parse(input: string): PaintOrder? {
        const s = trimSpaces(input);
        if (s.len == 0) { return null; }
        if (eqlIgnoreCaseAscii(s, "normal")) {
            return PaintOrder.normal();
        }

        let toks: [4]string = ["", "", "", ""];
        const n = splitBalancedSpace(s, toks[0..4]) ?? return null;
        if (n == 0 || n > 3) { return null; }

        let seen: [3]bool = [false, false, false];
        let out: [3]PaintOrderItem = [.fill, .stroke, .markers];
        let i: usize = 0;
        while (i < n) {
            const item = PaintOrderItem.fromStr(toks[i]) ?? return null;
            const slot = (item as u8) as usize;
            if (seen[slot]) { return null; } // 禁止重复
            seen[slot] = true;
            out[i] = item;
            i += 1;
        }

        // 按默认顺序 [fill, stroke, markers] 自动补全剩余项
        const defaults: [3]PaintOrderItem = [.fill, .stroke, .markers];
        let write_idx = n;
        let d: usize = 0;
        while (d < 3) {
            const slot = (defaults[d] as u8) as usize;
            if (!seen[slot]) {
                out[write_idx] = defaults[d];
                write_idx += 1;
            }
            d += 1;
        }

        return PaintOrder{ order: out };
    }

    fn isNormal(): bool {
        return _.order[0] == .fill && _.order[1] == .stroke && _.order[2] == .markers;
    }

    fn eql(other: PaintOrder): bool {
        return _.order[0] == other.order[0] && _.order[1] == other.order[1] && _.order[2] == other.order[2];
    }

    /// 规范最短序列化：`fill stroke markers` -> `"normal"`；尾部按默认序排列则省略
    fn toCss(out: []u8): []const u8 {
        let pos: usize = 0;
        if (_.isNormal()) {
            if (!appendStr(out, *pos, "normal")) { return out[0..0]; }
            return out[0..pos];
        }
        // 若第 2、3 项满足默认升序（fill < stroke < markers），则仅输出第 1 项即可唯一还原
        const second_ord = _.order[1] as u8;
        const third_ord = _.order[2] as u8;
        if (second_ord < third_ord) {
            if (!appendStr(out, *pos, _.order[0].asStr())) { return out[0..0]; }
            return out[0..pos];
        }
        // 否则输出前 2 项（第 3 项唯一确定）
        if (!appendStr(out, *pos, _.order[0].asStr())) { return out[0..0]; }
        if (!appendStr(out, *pos, " ")) { return out[0..0]; }
        if (!appendStr(out, *pos, _.order[1].asStr())) { return out[0..0]; }
        return out[0..pos];
    }
}

/// SVG `color-interpolation` 枚举
pub enum ColorInterpolation: u8 {
    auto = 0,
    srgb = 1,
    linear_rgb = 2,

    static fn fromStr(s: string): ColorInterpolation? {
        const t = trimSpaces(s);
        if (eqlIgnoreCaseAscii(t, "auto")) { return .auto; }
        if (eqlIgnoreCaseAscii(t, "sRGB")) { return .srgb; }
        if (eqlIgnoreCaseAscii(t, "linearRGB")) { return .linear_rgb; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .auto => "auto",
            .srgb => "sRGB",
            .linear_rgb => "linearRGB",
        };
    }
}

/// SVG `color-rendering` 枚举
pub enum ColorRendering: u8 {
    auto = 0,
    optimize_speed = 1,
    optimize_quality = 2,

    static fn fromStr(s: string): ColorRendering? {
        const t = trimSpaces(s);
        if (eqlIgnoreCaseAscii(t, "auto")) { return .auto; }
        if (eqlIgnoreCaseAscii(t, "optimizeSpeed")) { return .optimize_speed; }
        if (eqlIgnoreCaseAscii(t, "optimizeQuality")) { return .optimize_quality; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .auto => "auto",
            .optimize_speed => "optimizeSpeed",
            .optimize_quality => "optimizeQuality",
        };
    }
}

/// SVG `shape-rendering` 枚举
pub enum ShapeRendering: u8 {
    auto = 0,
    optimize_speed = 1,
    crisp_edges = 2,
    geometric_precision = 3,

    static fn fromStr(s: string): ShapeRendering? {
        const t = trimSpaces(s);
        if (eqlIgnoreCaseAscii(t, "auto")) { return .auto; }
        if (eqlIgnoreCaseAscii(t, "optimizeSpeed")) { return .optimize_speed; }
        if (eqlIgnoreCaseAscii(t, "crispEdges")) { return .crisp_edges; }
        if (eqlIgnoreCaseAscii(t, "geometricPrecision")) { return .geometric_precision; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .auto => "auto",
            .optimize_speed => "optimizeSpeed",
            .crisp_edges => "crispEdges",
            .geometric_precision => "geometricPrecision",
        };
    }
}

/// SVG `text-rendering` 枚举
pub enum TextRendering: u8 {
    auto = 0,
    optimize_speed = 1,
    optimize_legibility = 2,
    geometric_precision = 3,

    static fn fromStr(s: string): TextRendering? {
        const t = trimSpaces(s);
        if (eqlIgnoreCaseAscii(t, "auto")) { return .auto; }
        if (eqlIgnoreCaseAscii(t, "optimizeSpeed")) { return .optimize_speed; }
        if (eqlIgnoreCaseAscii(t, "optimizeLegibility")) { return .optimize_legibility; }
        if (eqlIgnoreCaseAscii(t, "geometricPrecision")) { return .geometric_precision; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .auto => "auto",
            .optimize_speed => "optimizeSpeed",
            .optimize_legibility => "optimizeLegibility",
            .geometric_precision => "geometricPrecision",
        };
    }
}

/// CSS/SVG `image-rendering` 枚举
pub enum ImageRendering: u8 {
    auto = 0,
    smooth = 1,
    high_quality = 2,
    crisp_edges = 3,
    pixelated = 4,

    static fn fromStr(s: string): ImageRendering? {
        const t = trimSpaces(s);
        if (eqlIgnoreCaseAscii(t, "auto")) { return .auto; }
        if (eqlIgnoreCaseAscii(t, "smooth")) { return .smooth; }
        if (eqlIgnoreCaseAscii(t, "high-quality")) { return .high_quality; }
        if (eqlIgnoreCaseAscii(t, "crisp-edges")) { return .crisp_edges; }
        if (eqlIgnoreCaseAscii(t, "pixelated")) { return .pixelated; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .auto => "auto",
            .smooth => "smooth",
            .high_quality => "high-quality",
            .crisp_edges => "crisp-edges",
            .pixelated => "pixelated",
        };
    }
}
