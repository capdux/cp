// Source: Absorbed from /data/dev/bun/src/css/properties/grid.cp | Synced: 2026-11-22T08:17:48Z
// Pure-CP zero-allocation W3C CSS Grid Layout Level 2 property parser & serializer:
// - `grid-template-columns` / `grid-template-rows` (`none`, `<track-size>`, `minmax()`,
//   `fit-content()`, `repeat(N | auto-fill | auto-fit, ...)`, `[line-name]` brackets)
// - `grid-template-areas` (`none` or quoted row strings with `.` null-cell tokens,
//   strict uniform column count validation, and W3C rectangular named-area verification).

/// 单条 `grid-template-*` 轨道列表允许的最大轨道项数量
pub const CSS_GRID_MAX_TRACKS: usize = 32;

const CSS_GRID_MAX_LINE_SLOTS: usize = 33;

/// 单个 `[...]` 网格线名列表允许的最大名称数量
pub const CSS_GRID_MAX_LINE_NAMES: usize = 8;

/// `grid-template-areas` 允许的最大网格单元格总数（`rows * columns`）
pub const CSS_GRID_MAX_AREA_CELLS: usize = 128;

/// CSS Grid 轨道与模板区域解析/序列化错误集
pub err CssGridError {
    EmptyInput,
    InvalidTrackBreadth,
    FlexNotAllowedInMin,
    NegativeFlex,
    InvalidRepeatCount,
    InvalidRepeatSyntax,
    InvalidLineNames,
    InvalidTemplateArea,
    UnevenAreaColumns,
    NonRectangularArea,
    TooManyTracks,
    TooManyLineNames,
    TooManyAreaCells,
    BufferTooSmall,
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0C;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
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

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn isAreaNameCodepoint(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-' || c >= 0x80;
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-' || c >= 0x80;
}

fn isValidLineIdent(s: []const u8): bool {
    if (s.len == 0 || !isIdentStart(s[0])) {
        return false;
    }
    if (eqlIgnoreCase(s, "span") || eqlIgnoreCase(s, "auto")) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        if (!isAreaNameCodepoint(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = ch;
    return pos + 1;
}

fn appendU32(out: []u8, pos: usize, val: u32): usize! {
    let tmp: [16]u8 = [0; 16];
    let len: usize = 0;
    let v = val;
    if (v == 0) {
        return appendChar(out, pos, '0');
    }
    while (v > 0) {
        tmp[len] = @intCast((v % 10) + '0');
        len += 1;
        v /= 10;
    }
    if (len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < len) {
        out[pos + i] = tmp[len - 1 - i];
        i += 1;
    }
    return pos + len;
}

fn findMatchingParen(s: []const u8, open_idx: usize): usize? {
    let depth: usize = 0;
    let i = open_idx;
    while (i < s.len) {
        if (s[i] == '(') {
            depth += 1;
        } else if (s[i] == ')') {
            if (depth == 0) return null;
            depth -= 1;
            if (depth == 0) return i;
        }
        i += 1;
    }
    return null;
}

fn findTopLevelComma(s: []const u8): usize? {
    let depth: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '(' || s[i] == '[') {
            depth += 1;
        } else if (s[i] == ')' || s[i] == ']') {
            if (depth > 0) depth -= 1;
        } else if (s[i] == ',' && depth == 0) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn isNumericPrefix(s: []const u8): usize {
    let i: usize = 0;
    if (i < s.len && (s[i] == '+' || s[i] == '-')) {
        i += 1;
    }
    let saw_digit = false;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        saw_digit = true;
        i += 1;
    }
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            saw_digit = true;
            i += 1;
        }
    }
    if (!saw_digit) {
        return 0;
    }
    return i;
}

fn isLengthPercentageToken(s: []const u8): bool {
    if (s.len == 0) return false;
    if (bytesEql(s, "0")) return true;
    const num_len = isNumericPrefix(s);
    if (num_len == 0 || num_len >= s.len) return false;
    const unit = s[num_len..];
    return bytesEql(unit, "%") ||
        eqlIgnoreCase(unit, "px") ||
        eqlIgnoreCase(unit, "em") ||
        eqlIgnoreCase(unit, "rem") ||
        eqlIgnoreCase(unit, "vw") ||
        eqlIgnoreCase(unit, "vh") ||
        eqlIgnoreCase(unit, "vmin") ||
        eqlIgnoreCase(unit, "vmax") ||
        eqlIgnoreCase(unit, "ch") ||
        eqlIgnoreCase(unit, "ex") ||
        eqlIgnoreCase(unit, "cm") ||
        eqlIgnoreCase(unit, "mm") ||
        eqlIgnoreCase(unit, "in") ||
        eqlIgnoreCase(unit, "pt") ||
        eqlIgnoreCase(unit, "pc") ||
        eqlIgnoreCase(unit, "cqw") ||
        eqlIgnoreCase(unit, "cqh") ||
        eqlIgnoreCase(unit, "svh") ||
        eqlIgnoreCase(unit, "lvh") ||
        eqlIgnoreCase(unit, "dvh");
}

fn parsePositiveU32(s: []const u8): u32? {
    if (s.len == 0) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] < '0' || s[i] > '9') return null;
        const d: u32 = @intCast(s[i] - '0');
        if (val > 400_000_000) return null;
        val = val * 10 + d;
        i += 1;
    }
    if (val == 0) return null;
    return val;
}

/// CSS `<track-breadth>` 类别
pub enum CssGridTrackBreadthKind: u8 {
    Auto = 0,
    MinContent = 1,
    MaxContent = 2,
    LengthPercentage = 3,
    Flex = 4,
}

/// CSS `<track-breadth>` 值（`auto` | `min-content` | `max-content` | `<length-percentage>` | `<flex>`）
pub struct CssGridTrackBreadth {
    kind: CssGridTrackBreadthKind = .Auto,
    raw: []const u8 = "auto",

    static fn parseInternal(input: []const u8, allow_flex: bool): CssGridTrackBreadth! {
        const s = trimWs(input);
        if (s.len == 0) {
            return err.InvalidTrackBreadth;
        }
        if (eqlIgnoreCase(s, "auto")) {
            return CssGridTrackBreadth{ kind: .Auto, raw: "auto" };
        }
        if (eqlIgnoreCase(s, "min-content")) {
            return CssGridTrackBreadth{ kind: .MinContent, raw: "min-content" };
        }
        if (eqlIgnoreCase(s, "max-content")) {
            return CssGridTrackBreadth{ kind: .MaxContent, raw: "max-content" };
        }
        const num_len = isNumericPrefix(s);
        if (num_len > 0 && num_len < s.len && eqlIgnoreCase(s[num_len..], "fr")) {
            if (s[0] == '-') {
                return err.NegativeFlex;
            }
            if (!allow_flex) {
                return err.FlexNotAllowedInMin;
            }
            return CssGridTrackBreadth{ kind: .Flex, raw: s };
        }
        if (isLengthPercentageToken(s)) {
            return CssGridTrackBreadth{ kind: .LengthPercentage, raw: s };
        }
        return err.InvalidTrackBreadth;
    }

    static fn parse(input: []const u8): CssGridTrackBreadth! {
        return CssGridTrackBreadth.parseInternal(input, true);
    }

    fn writeCss(out: []u8, pos: usize): usize! {
        return appendBytes(out, pos, _.raw);
    }
}

/// CSS `<track-size>` 类别
pub enum CssGridTrackSizeKind: u8 {
    Breadth = 0,
    MinMax = 1,
    FitContent = 2,
}

/// CSS `<track-size>` 值（`<track-breadth>` | `minmax(min, max)` | `fit-content(<length-percentage>)`）
pub struct CssGridTrackSize {
    kind: CssGridTrackSizeKind = .Breadth,
    breadth: CssGridTrackBreadth = CssGridTrackBreadth{},
    min_breadth: CssGridTrackBreadth = CssGridTrackBreadth{},
    max_breadth: CssGridTrackBreadth = CssGridTrackBreadth{},
    fit_arg: []const u8 = "",

    static fn parse(input: []const u8): CssGridTrackSize! {
        const s = trimWs(input);
        if (s.len == 0) {
            return err.EmptyInput;
        }
        // minmax(min, max)
        if (s.len >= 8 && eqlIgnoreCase(s[0..7], "minmax(") && s[s.len - 1] == ')') {
            const inner = s[7..s.len - 1];
            const comma = findTopLevelComma(inner) ?? return err.InvalidTrackBreadth;
            // Per W3C Grid Level 2 & Bun grid.cp: `<flex>` is invalid as `min` in `minmax()`
            const min_b = CssGridTrackBreadth.parseInternal(inner[0..comma], false)?;
            const max_b = CssGridTrackBreadth.parseInternal(inner[comma + 1..], true)?;
            return CssGridTrackSize{
                kind: .MinMax,
                min_breadth: min_b,
                max_breadth: max_b,
            };
        }
        // fit-content(<length-percentage>)
        if (s.len >= 13 && eqlIgnoreCase(s[0..12], "fit-content(") && s[s.len - 1] == ')') {
            const inner = trimWs(s[12..s.len - 1]);
            if (!isLengthPercentageToken(inner)) {
                return err.InvalidTrackBreadth;
            }
            return CssGridTrackSize{
                kind: .FitContent,
                fit_arg: inner,
            };
        }
        const b = CssGridTrackBreadth.parse(s)?;
        return CssGridTrackSize{
            kind: .Breadth,
            breadth: b,
        };
    }

    fn writeCss(out: []u8, pos: usize, minified: bool): usize! {
        let p = pos;
        if (_.kind == .Breadth) {
            return _.breadth.writeCss(out, p);
        }
        if (_.kind == .MinMax) {
            p = appendBytes(out, p, "minmax(")?;
            p = _.min_breadth.writeCss(out, p)?;
            p = appendBytes(out, p, if (minified) "," else ", ")?;
            p = _.max_breadth.writeCss(out, p)?;
            return appendChar(out, p, ')');
        }
        p = appendBytes(out, p, "fit-content(")?;
        p = appendBytes(out, p, _.fit_arg)?;
        return appendChar(out, p, ')');
    }
}

/// `repeat()` 计数类别（正整数 | `auto-fill` | `auto-fit`）
pub enum CssGridRepeatCountKind: u8 {
    Number = 0,
    AutoFill = 1,
    AutoFit = 2,
}

/// `repeat()` 计数参数
pub struct CssGridRepeatCount {
    kind: CssGridRepeatCountKind = .Number,
    number: u32 = 1,

    static fn parse(input: []const u8): CssGridRepeatCount! {
        const s = trimWs(input);
        if (eqlIgnoreCase(s, "auto-fill")) {
            return CssGridRepeatCount{ kind: .AutoFill, number: 0 };
        }
        if (eqlIgnoreCase(s, "auto-fit")) {
            return CssGridRepeatCount{ kind: .AutoFit, number: 0 };
        }
        if (n := parsePositiveU32(s)) {
            return CssGridRepeatCount{ kind: .Number, number: n };
        }
        return err.InvalidRepeatCount;
    }

    fn writeCss(out: []u8, pos: usize): usize! {
        if (_.kind == .AutoFill) return appendBytes(out, pos, "auto-fill");
        if (_.kind == .AutoFit) return appendBytes(out, pos, "auto-fit");
        return appendU32(out, pos, _.number);
    }
}

/// 单个 `[line-a line-b]` 网格线名集合
pub struct CssGridLineNameSet {
    names: [CSS_GRID_MAX_LINE_NAMES][]const u8 = [""; CSS_GRID_MAX_LINE_NAMES],
    count: usize = 0,

    fn isEmpty(): bool {
        return _.count == 0;
    }

    static fn parseBracketed(inner: []const u8): CssGridLineNameSet! {
        let set = CssGridLineNameSet{};
        let i: usize = 0;
        while (i < inner.len) {
            while (i < inner.len && isWs(inner[i])) {
                i += 1;
            }
            if (i >= inner.len) break;
            const start = i;
            while (i < inner.len && !isWs(inner[i])) {
                i += 1;
            }
            const tok = inner[start..i];
            if (!isValidLineIdent(tok)) {
                return err.InvalidLineNames;
            }
            if (set.count >= CSS_GRID_MAX_LINE_NAMES) {
                return err.TooManyLineNames;
            }
            set.names[set.count] = tok;
            set.count += 1;
        }
        return set;
    }

    fn writeCss(out: []u8, pos: usize): usize! {
        if (_.count == 0) return pos;
        let p = appendChar(out, pos, '[')?;
        let i: usize = 0;
        while (i < _.count) {
            if (i > 0) {
                p = appendChar(out, p, ' ')?;
            }
            p = appendBytes(out, p, _.names[i])?;
            i += 1;
        }
        return appendChar(out, p, ']');
    }
}

/// 单个 `repeat(count, track1 track2 ...)` 项
pub struct CssGridTrackRepeat {
    count: CssGridRepeatCount = CssGridRepeatCount{},
    tracks: [8]CssGridTrackSize = [CssGridTrackSize{}; 8],
    track_count: usize = 0,

    static fn parse(input: []const u8): CssGridTrackRepeat! {
        const s = trimWs(input);
        if (s.len < 9 || !eqlIgnoreCase(s[0..7], "repeat(") || s[s.len - 1] != ')') {
            return err.InvalidRepeatSyntax;
        }
        const inner = s[7..s.len - 1];
        const comma = findTopLevelComma(inner) ?? return err.InvalidRepeatSyntax;
        const cnt = CssGridRepeatCount.parse(inner[0..comma])?;
        const body = trimWs(inner[comma + 1..]);
        if (body.len == 0) {
            return err.InvalidRepeatSyntax;
        }

        let rep = CssGridTrackRepeat{
            count: cnt,
            tracks: [CssGridTrackSize{}; 8],
            track_count: 0,
        };
        let i: usize = 0;
        while (i < body.len) {
            while (i < body.len && isWs(body[i])) {
                i += 1;
            }
            if (i >= body.len) break;
            const start = i;
            let depth: usize = 0;
            while (i < body.len) {
                if (body[i] == '(') {
                    depth += 1;
                } else if (body[i] == ')') {
                    if (depth == 0) return err.InvalidRepeatSyntax;
                    depth -= 1;
                } else if (isWs(body[i]) && depth == 0) {
                    break;
                }
                i += 1;
            }
            if (depth != 0) return err.InvalidRepeatSyntax;
            if (rep.track_count >= 8) return err.TooManyTracks;
            rep.tracks[rep.track_count] = CssGridTrackSize.parse(body[start..i])?;
            rep.track_count += 1;
        }
        if (rep.track_count == 0) {
            return err.InvalidRepeatSyntax;
        }
        return rep;
    }

    fn writeCss(out: []u8, pos: usize, minified: bool): usize! {
        let p = appendBytes(out, pos, "repeat(")?;
        p = _.count.writeCss(out, p)?;
        p = appendBytes(out, p, if (minified) "," else ", ")?;
        let i: usize = 0;
        while (i < _.track_count) {
            if (i > 0) {
                p = appendChar(out, p, ' ')?;
            }
            p = _.tracks[i].writeCss(out, p, minified)?;
            i += 1;
        }
        return appendChar(out, p, ')');
    }
}

/// 轨道列表条目类别（单轨道尺寸或 `repeat()`）
pub enum CssGridTrackItemKind: u8 {
    TrackSize = 0,
    Repeat = 1,
}

/// 轨道列表中的单个条目
pub struct CssGridTrackItem {
    kind: CssGridTrackItemKind = .TrackSize,
    size: CssGridTrackSize = CssGridTrackSize{},
    repeat: CssGridTrackRepeat = CssGridTrackRepeat{},
}

/// `grid-template-columns` / `grid-template-rows` 解析结果（`none` 或 `<track-list>`）
pub struct CssGridTrackSizing {
    is_none: bool = false,
    line_names: [CSS_GRID_MAX_LINE_SLOTS]CssGridLineNameSet = [CssGridLineNameSet{}; CSS_GRID_MAX_LINE_SLOTS],
    items: [CSS_GRID_MAX_TRACKS]CssGridTrackItem = [CssGridTrackItem{}; CSS_GRID_MAX_TRACKS],
    item_count: usize = 0,

    static fn parse(input: []const u8): CssGridTrackSizing! {
        const s = trimWs(input);
        if (s.len == 0) {
            return err.EmptyInput;
        }
        if (eqlIgnoreCase(s, "none")) {
            return CssGridTrackSizing{ is_none: true };
        }

        let res = CssGridTrackSizing{ is_none: false };
        let i: usize = 0;

        while (true) {
            while (i < s.len && isWs(s[i])) {
                i += 1;
            }
            // Optional leading/between/trailing [line-names]
            if (i < s.len && s[i] == '[') {
                const close_b = blk: {
                    let j = i + 1;
                    while (j < s.len && s[j] != ']') {
                        j += 1;
                    }
                    break :blk j;
                };
                if (close_b >= s.len) {
                    return err.InvalidLineNames;
                }
                res.line_names[res.item_count] = CssGridLineNameSet.parseBracketed(s[i + 1..close_b])?;
                i = close_b + 1;
                while (i < s.len && isWs(s[i])) {
                    i += 1;
                }
            }

            if (i >= s.len) {
                break;
            }

            // Parse next track item (function or simple token)
            const start = i;
            let depth: usize = 0;
            while (i < s.len) {
                if (s[i] == '(') {
                    depth += 1;
                } else if (s[i] == ')') {
                    if (depth == 0) return err.InvalidTrackBreadth;
                    depth -= 1;
                } else if ((isWs(s[i]) || s[i] == '[') && depth == 0) {
                    break;
                }
                i += 1;
            }
            if (depth != 0) {
                return err.InvalidTrackBreadth;
            }

            const tok = s[start..i];
            if (res.item_count >= CSS_GRID_MAX_TRACKS) {
                return err.TooManyTracks;
            }
            if (tok.len >= 7 && eqlIgnoreCase(tok[0..7], "repeat(")) {
                const rep = CssGridTrackRepeat.parse(tok)?;
                res.items[res.item_count] = CssGridTrackItem{
                    kind: .Repeat,
                    repeat: rep,
                };
            } else {
                const ts = CssGridTrackSize.parse(tok)?;
                res.items[res.item_count] = CssGridTrackItem{
                    kind: .TrackSize,
                    size: ts,
                };
            }
            res.item_count += 1;
        }

        if (res.item_count == 0) {
            return err.InvalidTrackBreadth;
        }
        return res;
    }

    fn writeCss(out: []u8, minified: bool): usize! {
        if (_.is_none) {
            return appendBytes(out, 0, "none");
        }
        let pos: usize = 0;
        let i: usize = 0;
        while (i <= _.item_count) {
            const has_names = !_.line_names[i].isEmpty();
            if (has_names) {
                if (pos > 0 && !minified) {
                    pos = appendChar(out, pos, ' ')?;
                }
                pos = _.line_names[i].writeCss(out, pos)?;
            }
            if (i < _.item_count) {
                if (has_names) {
                    if (!minified) {
                        pos = appendChar(out, pos, ' ')?;
                    }
                } else if (pos > 0) {
                    pos = appendChar(out, pos, ' ')?;
                }
                const item = _.items[i];
                if (item.kind == .Repeat) {
                    pos = item.repeat.writeCss(out, pos, minified)?;
                } else {
                    pos = item.size.writeCss(out, pos, minified)?;
                }
            }
            i += 1;
        }
        return pos;
    }

    fn toCss(out: []u8): usize! {
        return _.writeCss(out, false);
    }

    fn toCssMinified(out: []u8): usize! {
        return _.writeCss(out, true);
    }
}

/// W3C `grid-template-areas` 解析与矩形校验结构
pub struct CssGridTemplateAreas {
    is_none: bool = false,
    rows: usize = 0,
    columns: usize = 0,
    /// 展平单元格名称数组；空字符串 `""` 代表 `.` 匿名空单元格
    cells: [CSS_GRID_MAX_AREA_CELLS][]const u8 = [""; CSS_GRID_MAX_AREA_CELLS],

    fn cellAt(row: usize, col: usize): ([]const u8)? {
        if (_.is_none || row >= _.rows || col >= _.columns) {
            return null;
        }
        const v = _.cells[row * _.columns + col];
        if (v.len == 0) {
            return null;
        }
        return v;
    }

    /// 校验所有命名区域是否严格形成连续实心矩形（W3C CSS Grid Level 2 规则）
    fn validateRectangles(): bool {
        let r: usize = 0;
        while (r < _.rows) {
            let c: usize = 0;
            while (c < _.columns) {
                const name = _.cells[r * _.columns + c];
                if (name.len > 0) {
                    let min_r = r;
                    let max_r = r;
                    let min_c = c;
                    let max_c = c;
                    let count: usize = 0;

                    let rr: usize = 0;
                    while (rr < _.rows) {
                        let cc: usize = 0;
                        while (cc < _.columns) {
                            if (bytesEql(_.cells[rr * _.columns + cc], name)) {
                                if (rr < min_r) min_r = rr;
                                if (rr > max_r) max_r = rr;
                                if (cc < min_c) min_c = cc;
                                if (cc > max_c) max_c = cc;
                                count += 1;
                            }
                            cc += 1;
                        }
                        rr += 1;
                    }

                    const box_area = (max_r - min_r + 1) * (max_c - min_c + 1);
                    if (count != box_area) {
                        return false;
                    }
                    // Verify every cell inside the bounding box matches `name`
                    rr = min_r;
                    while (rr <= max_r) {
                        let cc = min_c;
                        while (cc <= max_c) {
                            if (!bytesEql(_.cells[rr * _.columns + cc], name)) {
                                return false;
                            }
                            cc += 1;
                        }
                        rr += 1;
                    }
                }
                c += 1;
            }
            r += 1;
        }
        return true;
    }

    static fn parse(input: []const u8): CssGridTemplateAreas! {
        const s = trimWs(input);
        if (s.len == 0) {
            return err.EmptyInput;
        }
        if (eqlIgnoreCase(s, "none")) {
            return CssGridTemplateAreas{ is_none: true };
        }

        let res = CssGridTemplateAreas{ is_none: false };
        let i: usize = 0;
        let total_cells: usize = 0;

        while (i < s.len) {
            while (i < s.len && isWs(s[i])) {
                i += 1;
            }
            if (i >= s.len) break;
            if (s[i] != '"' && s[i] != '\'') {
                return err.InvalidTemplateArea;
            }
            const quote = s[i];
            i += 1;
            const row_start = i;
            while (i < s.len && s[i] != quote) {
                i += 1;
            }
            if (i >= s.len) {
                return err.InvalidTemplateArea;
            }
            const row_str = s[row_start..i];
            i += 1;

            // Tokenize cells in row_str
            let col: usize = 0;
            let k: usize = 0;
            while (k < row_str.len) {
                while (k < row_str.len && isWs(row_str[k])) {
                    k += 1;
                }
                if (k >= row_str.len) break;

                if (total_cells >= CSS_GRID_MAX_AREA_CELLS) {
                    return err.TooManyAreaCells;
                }
                if (row_str[k] == '.') {
                    while (k < row_str.len && row_str[k] == '.') {
                        k += 1;
                    }
                    res.cells[total_cells] = "";
                    total_cells += 1;
                    col += 1;
                } else if (isAreaNameCodepoint(row_str[k])) {
                    const tok_start = k;
                    while (k < row_str.len && isAreaNameCodepoint(row_str[k])) {
                        k += 1;
                    }
                    res.cells[total_cells] = row_str[tok_start..k];
                    total_cells += 1;
                    col += 1;
                } else {
                    return err.InvalidTemplateArea;
                }
            }

            if (col == 0) {
                return err.InvalidTemplateArea;
            }
            if (res.rows == 0) {
                res.columns = col;
            } else if (col != res.columns) {
                return err.UnevenAreaColumns;
            }
            res.rows += 1;
        }

        if (res.rows == 0) {
            return err.InvalidTemplateArea;
        }
        if (!res.validateRectangles()) {
            return err.NonRectangularArea;
        }
        return res;
    }

    fn toCss(out: []u8): usize! {
        if (_.is_none) {
            return appendBytes(out, 0, "none");
        }
        let pos: usize = 0;
        let r: usize = 0;
        while (r < _.rows) {
            if (r > 0) {
                pos = appendChar(out, pos, ' ')?;
            }
            pos = appendChar(out, pos, '"')?;
            let c: usize = 0;
            while (c < _.columns) {
                if (c > 0) {
                    pos = appendChar(out, pos, ' ')?;
                }
                const cell = _.cells[r * _.columns + c];
                if (cell.len == 0) {
                    pos = appendChar(out, pos, '.')?;
                } else {
                    pos = appendBytes(out, pos, cell)?;
                }
                c += 1;
            }
            pos = appendChar(out, pos, '"')?;
            r += 1;
        }
        return pos;
    }
}
