// Source: Absorbed from /data/dev/bun/src/css/properties/border_image.cp | Synced: 2026-11-22T02:33:54Z

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

fn isNear(a: f64, b: f64): bool {
    return @abs(a - b) <= 0.000001;
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
    let val = val_in;
    if (@abs(val) <= 0.0000005) {
        return appendStr(buf, pos, "0");
    }
    let tmp: [64]u8 = [0; 64];
    let p: usize = 0;
    const scaled = @round(val * 1000000.0);
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

/// 括号平衡顶层词法切分（独立 '/' 作为单独 token，最多返回 out.len 个 token）
fn tokenizeTopLevel(s: string, out: []string): usize? {
    let count: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        while (i < s.len && isSpace(s[i])) { i += 1; }
        if (i >= s.len) { break; }

        if (s[i] == '/') {
            if (count >= out.len) { return null; }
            out[count] = s[i .. i + 1];
            count += 1;
            i += 1;
            continue;
        }

        const start = i;
        let depth: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (c == '(') {
                depth += 1;
            } else if (c == ')') {
                if (depth == 0) { return null; }
                depth -= 1;
            } else if (depth == 0 && (isSpace(c) || c == '/')) {
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

/// `border-image-repeat` 关键词枚举
pub enum BorderImageRepeatKeyword: u8 {
    stretch = 0,
    repeat = 1,
    round = 2,
    space = 3,

    static fn fromStr(s: string): BorderImageRepeatKeyword? {
        if (eqlIgnoreCaseAscii(s, "stretch")) { return .stretch; }
        if (eqlIgnoreCaseAscii(s, "repeat")) { return .repeat; }
        if (eqlIgnoreCaseAscii(s, "round")) { return .round; }
        if (eqlIgnoreCaseAscii(s, "space")) { return .space; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .stretch => "stretch",
            .repeat => "repeat",
            .round => "round",
            .space => "space",
        };
    }
}

/// `border-image-repeat` 双轴配置（默认 `stretch stretch`）
pub struct BorderImageRepeat {
    horizontal: BorderImageRepeatKeyword = .stretch,
    vertical: BorderImageRepeatKeyword = .stretch,

    static fn defaultRepeat(): BorderImageRepeat {
        return BorderImageRepeat{};
    }

    static fn parse(input: string): BorderImageRepeat? {
        let toks: [4]string = ["", "", "", ""];
        const n = tokenizeTopLevel(input, toks[0..4]) ?? return null;
        if (n == 0 || n > 2) { return null; }
        const h = BorderImageRepeatKeyword.fromStr(toks[0]) ?? return null;
        const v = if (n == 2) (BorderImageRepeatKeyword.fromStr(toks[1]) ?? return null) else h;
        return BorderImageRepeat{ horizontal: h, vertical: v };
    }

    fn isDefault(): bool {
        return _.horizontal == .stretch && _.vertical == .stretch;
    }

    fn eql(other: BorderImageRepeat): bool {
        return _.horizontal == other.horizontal && _.vertical == other.vertical;
    }

    fn writeTo(buf: []u8, pos: *usize): bool {
        if (!appendStr(buf, pos, _.horizontal.asStr())) { return false; }
        if (_.horizontal != _.vertical) {
            if (!appendStr(buf, pos, " ")) { return false; }
            if (!appendStr(buf, pos, _.vertical.asStr())) { return false; }
        }
        return true;
    }

    fn toCss(out: []u8): []const u8 {
        let pos: usize = 0;
        if (!_.writeTo(out, *pos)) { return out[0..0]; }
        return out[0..pos];
    }
}

/// `border-image-slice` 单边偏移（非负无单位数或百分比）
pub struct SliceOffset {
    value: f64 = 100.0,
    is_percent: bool = true,

    static fn percent(v: f64): SliceOffset {
        return SliceOffset{ value: v, is_percent: true };
    }

    static fn number(v: f64): SliceOffset {
        return SliceOffset{ value: v, is_percent: false };
    }

    static fn parse(tok: string): SliceOffset? {
        const s = trimSpaces(tok);
        if (s.len == 0) { return null; }
        if (s[s.len - 1] == '%') {
            const v = parseNonNegFloat(s[0 .. s.len - 1]) ?? return null;
            return SliceOffset.percent(v);
        }
        const v = parseNonNegFloat(s) ?? return null;
        return SliceOffset.number(v);
    }

    fn eql(other: SliceOffset): bool {
        return _.is_percent == other.is_percent && isNear(_.value, other.value);
    }

    fn writeTo(buf: []u8, pos: *usize): bool {
        if (!appendNum(buf, pos, _.value)) { return false; }
        if (_.is_percent) {
            if (!appendStr(buf, pos, "%")) { return false; }
        }
        return true;
    }
}

/// `border-image-slice` 1~4 边偏移 + `fill` 标志（默认四边 `100%`, `fill = false`）
pub struct BorderImageSlice {
    top: SliceOffset = SliceOffset{},
    right: SliceOffset = SliceOffset{},
    bottom: SliceOffset = SliceOffset{},
    left: SliceOffset = SliceOffset{},
    fill: bool = false,

    static fn defaultSlice(): BorderImageSlice {
        return BorderImageSlice{};
    }

    static fn parse(input: string): BorderImageSlice? {
        let toks: [8]string = [""; 8];
        const n = tokenizeTopLevel(input, toks[0..8]) ?? return null;
        if (n == 0) { return null; }
        let idx: usize = 0;
        const res = parseSliceFromTokens(toks[0..n], *idx) ?? return null;
        if (idx != n) { return null; }
        return res;
    }

    fn isDefault(): bool {
        const d = SliceOffset.percent(100.0);
        return !_.fill && _.top.eql(d) && _.right.eql(d) && _.bottom.eql(d) && _.left.eql(d);
    }

    fn eql(other: BorderImageSlice): bool {
        return _.fill == other.fill &&
            _.top.eql(other.top) &&
            _.right.eql(other.right) &&
            _.bottom.eql(other.bottom) &&
            _.left.eql(other.left);
    }

    fn writeTo(buf: []u8, pos: *usize): bool {
        const same_v = _.top.eql(_.bottom);
        const same_h = _.right.eql(_.left);

        if (!_.top.writeTo(buf, pos)) { return false; }
        if (!(same_v && same_h && _.top.eql(_.right))) {
            if (!appendStr(buf, pos, " ")) { return false; }
            if (!_.right.writeTo(buf, pos)) { return false; }
            if (!(same_v && same_h)) {
                if (!appendStr(buf, pos, " ")) { return false; }
                if (!_.bottom.writeTo(buf, pos)) { return false; }
                if (!same_h) {
                    if (!appendStr(buf, pos, " ")) { return false; }
                    if (!_.left.writeTo(buf, pos)) { return false; }
                }
            }
        }
        if (_.fill) {
            if (!appendStr(buf, pos, " fill")) { return false; }
        }
        return true;
    }

    fn toCss(out: []u8): []const u8 {
        let pos: usize = 0;
        if (!_.writeTo(out, *pos)) { return out[0..0]; }
        return out[0..pos];
    }
}

fn parseSliceFromTokens(toks: []const string, idx: *usize): BorderImageSlice? {
    let seen_fill = false;
    let offsets: [4]SliceOffset = [SliceOffset{}; 4];
    let count: usize = 0;
    const start_idx = idx.*;

    if (idx.* < toks.len && eqlIgnoreCaseAscii(toks[idx.*], "fill")) {
        seen_fill = true;
        idx.* += 1;
    }

    while (idx.* < toks.len && count < 4) {
        if (off := SliceOffset.parse(toks[idx.*])) {
            offsets[count] = off;
            count += 1;
            idx.* += 1;
        } else {
            break;
        }
    }

    if (count == 0) {
        idx.* = start_idx;
        return null;
    }

    if (!seen_fill && idx.* < toks.len && eqlIgnoreCaseAscii(toks[idx.*], "fill")) {
        seen_fill = true;
        idx.* += 1;
    }

    if (count == 1) {
        return BorderImageSlice{ top: offsets[0], right: offsets[0], bottom: offsets[0], left: offsets[0], fill: seen_fill };
    }
    if (count == 2) {
        return BorderImageSlice{ top: offsets[0], right: offsets[1], bottom: offsets[0], left: offsets[1], fill: seen_fill };
    }
    if (count == 3) {
        return BorderImageSlice{ top: offsets[0], right: offsets[1], bottom: offsets[2], left: offsets[1], fill: seen_fill };
    }
    return BorderImageSlice{ top: offsets[0], right: offsets[1], bottom: offsets[2], left: offsets[3], fill: seen_fill };
}

/// `border-image-width` 单边类型
pub enum WidthKind: u8 {
    auto_kw = 0,
    number = 1,
    length_px = 2,
    percent = 3,
}

/// `border-image-width` 单边宽度（`auto` / 非负无单位数 / 非负 px / 非负百分比）
pub struct BorderImageSideWidth {
    kind: WidthKind = .number,
    value: f64 = 1.0,

    static fn autoVal(): BorderImageSideWidth {
        return BorderImageSideWidth{ kind: .auto_kw, value: 0.0 };
    }

    static fn number(v: f64): BorderImageSideWidth {
        return BorderImageSideWidth{ kind: .number, value: v };
    }

    static fn px(v: f64): BorderImageSideWidth {
        return BorderImageSideWidth{ kind: .length_px, value: v };
    }

    static fn percent(v: f64): BorderImageSideWidth {
        return BorderImageSideWidth{ kind: .percent, value: v };
    }

    static fn parse(tok: string): BorderImageSideWidth? {
        const s = trimSpaces(tok);
        if (s.len == 0) { return null; }
        if (eqlIgnoreCaseAscii(s, "auto")) {
            return BorderImageSideWidth.autoVal();
        }
        if (s[s.len - 1] == '%') {
            const v = parseNonNegFloat(s[0 .. s.len - 1]) ?? return null;
            return BorderImageSideWidth.percent(v);
        }
        if (s.len >= 2 && eqlIgnoreCaseAscii(s[s.len - 2 ..], "px")) {
            const v = parseNonNegFloat(s[0 .. s.len - 2]) ?? return null;
            return BorderImageSideWidth.px(v);
        }
        const v = parseNonNegFloat(s) ?? return null;
        return BorderImageSideWidth.number(v);
    }

    fn eql(other: BorderImageSideWidth): bool {
        if (_.kind != other.kind) { return false; }
        if (_.kind == .auto_kw) { return true; }
        return isNear(_.value, other.value);
    }

    fn writeTo(buf: []u8, pos: *usize): bool {
        return switch (_.kind) {
            .auto_kw => appendStr(buf, pos, "auto"),
            .number => appendNum(buf, pos, _.value),
            .length_px => {
                if (!appendNum(buf, pos, _.value)) { return false; }
                return appendStr(buf, pos, "px");
            },
            .percent => {
                if (!appendNum(buf, pos, _.value)) { return false; }
                return appendStr(buf, pos, "%");
            },
        };
    }
}

/// `border-image-width` 1~4 边简写（默认四边 `1`）
pub struct BorderImageWidth {
    top: BorderImageSideWidth = BorderImageSideWidth{},
    right: BorderImageSideWidth = BorderImageSideWidth{},
    bottom: BorderImageSideWidth = BorderImageSideWidth{},
    left: BorderImageSideWidth = BorderImageSideWidth{},

    static fn defaultWidth(): BorderImageWidth {
        return BorderImageWidth{};
    }

    static fn parse(input: string): BorderImageWidth? {
        let toks: [6]string = [""; 6];
        const n = tokenizeTopLevel(input, toks[0..6]) ?? return null;
        if (n == 0) { return null; }
        let idx: usize = 0;
        const res = parseWidthFromTokens(toks[0..n], *idx) ?? return null;
        if (idx != n) { return null; }
        return res;
    }

    fn isDefault(): bool {
        const d = BorderImageSideWidth.number(1.0);
        return _.top.eql(d) && _.right.eql(d) && _.bottom.eql(d) && _.left.eql(d);
    }

    fn eql(other: BorderImageWidth): bool {
        return _.top.eql(other.top) && _.right.eql(other.right) && _.bottom.eql(other.bottom) && _.left.eql(other.left);
    }

    fn writeTo(buf: []u8, pos: *usize): bool {
        const same_v = _.top.eql(_.bottom);
        const same_h = _.right.eql(_.left);

        if (!_.top.writeTo(buf, pos)) { return false; }
        if (!(same_v && same_h && _.top.eql(_.right))) {
            if (!appendStr(buf, pos, " ")) { return false; }
            if (!_.right.writeTo(buf, pos)) { return false; }
            if (!(same_v && same_h)) {
                if (!appendStr(buf, pos, " ")) { return false; }
                if (!_.bottom.writeTo(buf, pos)) { return false; }
                if (!same_h) {
                    if (!appendStr(buf, pos, " ")) { return false; }
                    if (!_.left.writeTo(buf, pos)) { return false; }
                }
            }
        }
        return true;
    }

    fn toCss(out: []u8): []const u8 {
        let pos: usize = 0;
        if (!_.writeTo(out, *pos)) { return out[0..0]; }
        return out[0..pos];
    }
}

fn parseWidthFromTokens(toks: []const string, idx: *usize): BorderImageWidth? {
    let vals: [4]BorderImageSideWidth = [BorderImageSideWidth{}; 4];
    let count: usize = 0;
    while (idx.* < toks.len && count < 4) {
        if (w := BorderImageSideWidth.parse(toks[idx.*])) {
            vals[count] = w;
            count += 1;
            idx.* += 1;
        } else {
            break;
        }
    }
    if (count == 0) { return null; }
    if (count == 1) { return BorderImageWidth{ top: vals[0], right: vals[0], bottom: vals[0], left: vals[0] }; }
    if (count == 2) { return BorderImageWidth{ top: vals[0], right: vals[1], bottom: vals[0], left: vals[1] }; }
    if (count == 3) { return BorderImageWidth{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[1] }; }
    return BorderImageWidth{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[3] };
}

/// `border-image-outset` 单边类型
pub enum OutsetKind: u8 {
    number = 0,
    length_px = 1,
}

/// `border-image-outset` 单边外扩值（非负无单位数或非负 px 长度）
pub struct BorderImageOutsetValue {
    kind: OutsetKind = .number,
    value: f64 = 0.0,

    static fn number(v: f64): BorderImageOutsetValue {
        return BorderImageOutsetValue{ kind: .number, value: v };
    }

    static fn px(v: f64): BorderImageOutsetValue {
        return BorderImageOutsetValue{ kind: .length_px, value: v };
    }

    static fn parse(tok: string): BorderImageOutsetValue? {
        const s = trimSpaces(tok);
        if (s.len == 0) { return null; }
        if (s.len >= 2 && eqlIgnoreCaseAscii(s[s.len - 2 ..], "px")) {
            const v = parseNonNegFloat(s[0 .. s.len - 2]) ?? return null;
            return BorderImageOutsetValue.px(v);
        }
        const v = parseNonNegFloat(s) ?? return null;
        return BorderImageOutsetValue.number(v);
    }

    fn eql(other: BorderImageOutsetValue): bool {
        if (isNear(_.value, 0.0) && isNear(other.value, 0.0)) { return true; }
        return _.kind == other.kind && isNear(_.value, other.value);
    }

    fn writeTo(buf: []u8, pos: *usize): bool {
        if (!appendNum(buf, pos, _.value)) { return false; }
        if (_.kind == .length_px && !isNear(_.value, 0.0)) {
            if (!appendStr(buf, pos, "px")) { return false; }
        }
        return true;
    }
}

/// `border-image-outset` 1~4 边简写（默认四边 `0`）
pub struct BorderImageOutset {
    top: BorderImageOutsetValue = BorderImageOutsetValue{},
    right: BorderImageOutsetValue = BorderImageOutsetValue{},
    bottom: BorderImageOutsetValue = BorderImageOutsetValue{},
    left: BorderImageOutsetValue = BorderImageOutsetValue{},

    static fn defaultOutset(): BorderImageOutset {
        return BorderImageOutset{};
    }

    static fn parse(input: string): BorderImageOutset? {
        let toks: [6]string = [""; 6];
        const n = tokenizeTopLevel(input, toks[0..6]) ?? return null;
        if (n == 0) { return null; }
        let idx: usize = 0;
        const res = parseOutsetFromTokens(toks[0..n], *idx) ?? return null;
        if (idx != n) { return null; }
        return res;
    }

    fn isDefault(): bool {
        const d = BorderImageOutsetValue.number(0.0);
        return _.top.eql(d) && _.right.eql(d) && _.bottom.eql(d) && _.left.eql(d);
    }

    fn eql(other: BorderImageOutset): bool {
        return _.top.eql(other.top) && _.right.eql(other.right) && _.bottom.eql(other.bottom) && _.left.eql(other.left);
    }

    fn writeTo(buf: []u8, pos: *usize): bool {
        const same_v = _.top.eql(_.bottom);
        const same_h = _.right.eql(_.left);

        if (!_.top.writeTo(buf, pos)) { return false; }
        if (!(same_v && same_h && _.top.eql(_.right))) {
            if (!appendStr(buf, pos, " ")) { return false; }
            if (!_.right.writeTo(buf, pos)) { return false; }
            if (!(same_v && same_h)) {
                if (!appendStr(buf, pos, " ")) { return false; }
                if (!_.bottom.writeTo(buf, pos)) { return false; }
                if (!same_h) {
                    if (!appendStr(buf, pos, " ")) { return false; }
                    if (!_.left.writeTo(buf, pos)) { return false; }
                }
            }
        }
        return true;
    }

    fn toCss(out: []u8): []const u8 {
        let pos: usize = 0;
        if (!_.writeTo(out, *pos)) { return out[0..0]; }
        return out[0..pos];
    }
}

fn parseOutsetFromTokens(toks: []const string, idx: *usize): BorderImageOutset? {
    let vals: [4]BorderImageOutsetValue = [BorderImageOutsetValue{}; 4];
    let count: usize = 0;
    while (idx.* < toks.len && count < 4) {
        if (o := BorderImageOutsetValue.parse(toks[idx.*])) {
            vals[count] = o;
            count += 1;
            idx.* += 1;
        } else {
            break;
        }
    }
    if (count == 0) { return null; }
    if (count == 1) { return BorderImageOutset{ top: vals[0], right: vals[0], bottom: vals[0], left: vals[0] }; }
    if (count == 2) { return BorderImageOutset{ top: vals[0], right: vals[1], bottom: vals[0], left: vals[1] }; }
    if (count == 3) { return BorderImageOutset{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[1] }; }
    return BorderImageOutset{ top: vals[0], right: vals[1], bottom: vals[2], left: vals[3] };
}

fn isSourceToken(tok: string): bool {
    if (eqlIgnoreCaseAscii(tok, "none")) {
        return true;
    }
    if (tok.len >= 3 && tok[tok.len - 1] == ')') {
        let i: usize = 0;
        while (i < tok.len) {
            if (tok[i] == '(') {
                return i > 0;
            }
            i += 1;
        }
    }
    return false;
}

/// CSS `border-image` 完整简写属性（支持任意顶层顺序与双斜杠 `slice / width / outset`）
pub struct BorderImage {
    source: string = "none",
    slice: BorderImageSlice = BorderImageSlice.defaultSlice(),
    width: BorderImageWidth = BorderImageWidth.defaultWidth(),
    outset: BorderImageOutset = BorderImageOutset.defaultOutset(),
    repeat: BorderImageRepeat = BorderImageRepeat.defaultRepeat(),

    static fn parse(input: []const u8): BorderImage? {
        let toks: [24]string = [""; 24];
        const n = tokenizeTopLevel(input, toks[0..24]) ?? return null;
        if (n == 0) { return null; }

        let bi = BorderImage{};
        let has_source = false;
        let has_slice = false;
        let has_repeat = false;

        let idx: usize = 0;
        while (idx < n) {
            const tok = toks[idx];

            if (!has_source && isSourceToken(tok)) {
                bi.source = tok;
                has_source = true;
                idx += 1;
                continue;
            }

            if (!has_repeat) {
                if (h := BorderImageRepeatKeyword.fromStr(tok)) {
                    idx += 1;
                    let v = h;
                    if (idx < n) {
                        if (v2 := BorderImageRepeatKeyword.fromStr(toks[idx])) {
                            v = v2;
                            idx += 1;
                        }
                    }
                    bi.repeat = BorderImageRepeat{ horizontal: h, vertical: v };
                    has_repeat = true;
                    continue;
                }
            }

            if (!has_slice) {
                if (sl := parseSliceFromTokens(toks[0..n], *idx)) {
                    bi.slice = sl;
                    has_slice = true;

                    // 可选 `/ width [/ outset]` 或 `/ / outset`
                    if (idx < n && std.mem.eql(u8, toks[idx], "/")) {
                        idx += 1;
                        let parsed_w = false;
                        let parsed_o = false;

                        if (w := parseWidthFromTokens(toks[0..n], *idx)) {
                            bi.width = w;
                            parsed_w = true;
                        }

                        if (idx < n && std.mem.eql(u8, toks[idx], "/")) {
                            idx += 1;
                            if (o := parseOutsetFromTokens(toks[0..n], *idx)) {
                                bi.outset = o;
                                parsed_o = true;
                            } else {
                                return null;
                            }
                        }

                        if (!parsed_w && !parsed_o) {
                            return null;
                        }
                    }
                    continue;
                }
            }

            return null;
        }

        if (!has_source && !has_slice && !has_repeat) {
            return null;
        }
        return bi;
    }

    /// 规范化 CSS 序列化（自动省略默认值）
    fn toCss(out: []u8): []const u8 {
        let pos: usize = 0;
        let wrote_any = false;

        const has_src = !eqlIgnoreCaseAscii(_.source, "none");
        const has_slice = !_.slice.isDefault();
        const has_width = !_.width.isDefault();
        const has_outset = !_.outset.isDefault();
        const has_repeat = !_.repeat.isDefault();

        if (!has_src && !has_slice && !has_width && !has_outset && !has_repeat) {
            if (!appendStr(out, *pos, "none")) { return out[0..0]; }
            return out[0..pos];
        }

        if (has_src) {
            if (!appendStr(out, *pos, _.source)) { return out[0..0]; }
            wrote_any = true;
        }

        if (has_slice || has_width || has_outset) {
            if (wrote_any) {
                if (!appendStr(out, *pos, " ")) { return out[0..0]; }
            }
            if (!_.slice.writeTo(out, *pos)) { return out[0..0]; }
            if (has_width || has_outset) {
                if (!appendStr(out, *pos, " / ")) { return out[0..0]; }
            }
            if (has_width) {
                if (!_.width.writeTo(out, *pos)) { return out[0..0]; }
            }
            if (has_outset) {
                if (has_width) {
                    if (!appendStr(out, *pos, " / ")) { return out[0..0]; }
                } else {
                    if (!appendStr(out, *pos, "/ ")) { return out[0..0]; }
                }
                if (!_.outset.writeTo(out, *pos)) { return out[0..0]; }
            }
            wrote_any = true;
        }

        if (has_repeat) {
            if (wrote_any) {
                if (!appendStr(out, *pos, " ")) { return out[0..0]; }
            }
            if (!_.repeat.writeTo(out, *pos)) { return out[0..0]; }
        }

        return out[0..pos];
    }
}
