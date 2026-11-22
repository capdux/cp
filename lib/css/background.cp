// Source: Absorbed from /data/dev/bun/src/css/properties/background.cp | Synced: 2026-11-22T01:25:58Z

/// CSS Background 属性族解析错误集
pub err CssBackgroundError {
    InvalidRepeat,
    InvalidSize,
    InvalidPosition,
    InvalidAttachment,
    InvalidOrigin,
    InvalidClip,
    InvalidBackground,
    TooManyLayers,
    BufferTooSmall,
}

fn trimAscii(s: string): string {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff <= 0.000001;
}

/// `background-repeat` 单轴关键字
pub enum BackgroundRepeatKeyword: u8 {
    repeat = 0,
    space = 1,
    round = 2,
    no_repeat = 3,

    static fn parse(input: string): BackgroundRepeatKeyword? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "repeat")) { return .repeat; }
        if (std.ascii.eqlIgnoreCase(s, "space")) { return .space; }
        if (std.ascii.eqlIgnoreCase(s, "round")) { return .round; }
        if (std.ascii.eqlIgnoreCase(s, "no-repeat")) { return .no_repeat; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .repeat => "repeat",
            .space => "space",
            .round => "round",
            .no_repeat => "no-repeat",
        };
    }
}

/// `background-repeat` 双轴模型（支持 `repeat-x` / `repeat-y` 与双轴关键字）
pub struct BackgroundRepeat {
    x: BackgroundRepeatKeyword = BackgroundRepeatKeyword.repeat,
    y: BackgroundRepeatKeyword = BackgroundRepeatKeyword.repeat,

    static fn default(): BackgroundRepeat {
        return BackgroundRepeat{
            x: BackgroundRepeatKeyword.repeat,
            y: BackgroundRepeatKeyword.repeat,
        };
    }

    fn isDefault(): bool {
        return _.x == BackgroundRepeatKeyword.repeat && _.y == BackgroundRepeatKeyword.repeat;
    }

    fn eql(other: BackgroundRepeat): bool {
        return _.x == other.x && _.y == other.y;
    }

    static fn parse(input: string): BackgroundRepeat ! CssBackgroundError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidRepeat; }
        if (std.ascii.eqlIgnoreCase(s, "repeat-x")) {
            return BackgroundRepeat{ x: BackgroundRepeatKeyword.repeat, y: BackgroundRepeatKeyword.no_repeat };
        }
        if (std.ascii.eqlIgnoreCase(s, "repeat-y")) {
            return BackgroundRepeat{ x: BackgroundRepeatKeyword.no_repeat, y: BackgroundRepeatKeyword.repeat };
        }
        if (sp := std.mem.indexOfScalar(u8, s, ' ')) {
            const left = trimAscii(s[0..sp]);
            const right = trimAscii(s[sp + 1 ..]);
            if (std.mem.indexOfScalar(u8, right, ' ') != null) {
                return err.InvalidRepeat;
            }
            const kx = BackgroundRepeatKeyword.parse(left) ?? return err.InvalidRepeat;
            const ky = BackgroundRepeatKeyword.parse(right) ?? return err.InvalidRepeat;
            return BackgroundRepeat{ x: kx, y: ky };
        }
        const k = BackgroundRepeatKeyword.parse(s) ?? return err.InvalidRepeat;
        return BackgroundRepeat{ x: k, y: k };
    }

    fn toCss(buf: []u8): usize ! CssBackgroundError {
        const formatted = if (_.x == BackgroundRepeatKeyword.repeat && _.y == BackgroundRepeatKeyword.no_repeat)
            std.fmt.bufPrint(buf, "repeat-x", .{})
        else if (_.x == BackgroundRepeatKeyword.no_repeat && _.y == BackgroundRepeatKeyword.repeat)
            std.fmt.bufPrint(buf, "repeat-y", .{})
        else if (_.x == _.y)
            std.fmt.bufPrint(buf, "{s}", .{_.x.toCss()})
        else
            std.fmt.bufPrint(buf, "{s} {s}", .{ _.x.toCss(), _.y.toCss() });
        const slice = formatted catch return err.BufferTooSmall;
        return slice.len;
    }
}

/// 长度 / 百分比 / `auto` 类型
pub enum LengthPercentageAutoKind: u8 {
    auto = 0,
    px = 1,
    percent = 2,
}

/// 长度 / 百分比 / `auto` 标量值
pub struct LengthPercentageAuto {
    kind: LengthPercentageAutoKind = LengthPercentageAutoKind.auto,
    value: f64 = 0.0,

    static fn auto(): LengthPercentageAuto {
        return LengthPercentageAuto{ kind: LengthPercentageAutoKind.auto, value: 0.0 };
    }

    static fn px(v: f64): LengthPercentageAuto {
        return LengthPercentageAuto{ kind: LengthPercentageAutoKind.px, value: v };
    }

    static fn percent(v: f64): LengthPercentageAuto {
        return LengthPercentageAuto{ kind: LengthPercentageAutoKind.percent, value: v };
    }

    fn isAuto(): bool {
        return _.kind == LengthPercentageAutoKind.auto;
    }

    fn isZero(): bool {
        return _.kind != LengthPercentageAutoKind.auto && approxEq(_.value, 0.0);
    }

    fn eql(other: LengthPercentageAuto): bool {
        return _.kind == other.kind && approxEq(_.value, other.value);
    }

    static fn parse(input: string, allow_negative: bool): LengthPercentageAuto? {
        const s = trimAscii(input);
        if (s.len == 0) { return null; }
        if (std.ascii.eqlIgnoreCase(s, "auto")) { return LengthPercentageAuto.auto(); }
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "px")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 2]) catch return null;
            if (std.math.isNan(v) || std.math.isInf(v) || (!allow_negative && v < 0.0)) { return null; }
            return LengthPercentageAuto.px(v);
        }
        if (s.len > 1 && s[s.len - 1] == '%') {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 1]) catch return null;
            if (std.math.isNan(v) || std.math.isInf(v) || (!allow_negative && v < 0.0)) { return null; }
            return LengthPercentageAuto.percent(v);
        }
        const z = std.fmt.parseFloat(f64, s) catch return null;
        if (z == 0.0) {
            return LengthPercentageAuto.px(0.0);
        }
        return null;
    }

    fn toCss(buf: []u8): usize ! CssBackgroundError {
        const res = switch (_.kind) {
            .auto => std.fmt.bufPrint(buf, "auto", .{}),
            .px => if (approxEq(_.value, 0.0)) std.fmt.bufPrint(buf, "0", .{}) else std.fmt.bufPrint(buf, "{d}px", .{_.value}),
            .percent => std.fmt.bufPrint(buf, "{d}%", .{_.value}),
        } catch return err.BufferTooSmall;
        return res.len;
    }
}

/// `background-size` 种类
pub enum BackgroundSizeKind: u8 {
    explicit = 0,
    cover = 1,
    contain = 2,
}

/// `background-size` 模型（`cover` / `contain` / 显式宽高）
pub struct BackgroundSize {
    kind: BackgroundSizeKind = BackgroundSizeKind.explicit,
    width: LengthPercentageAuto = LengthPercentageAuto.auto(),
    height: LengthPercentageAuto = LengthPercentageAuto.auto(),

    static fn default(): BackgroundSize {
        return BackgroundSize{};
    }

    static fn cover(): BackgroundSize {
        return BackgroundSize{ kind: BackgroundSizeKind.cover };
    }

    static fn contain(): BackgroundSize {
        return BackgroundSize{ kind: BackgroundSizeKind.contain };
    }

    fn isDefault(): bool {
        return _.kind == BackgroundSizeKind.explicit && _.width.isAuto() && _.height.isAuto();
    }

    static fn parse(input: string): BackgroundSize ! CssBackgroundError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidSize; }
        if (std.ascii.eqlIgnoreCase(s, "cover")) { return BackgroundSize.cover(); }
        if (std.ascii.eqlIgnoreCase(s, "contain")) { return BackgroundSize.contain(); }
        if (sp := std.mem.indexOfScalar(u8, s, ' ')) {
            const left = trimAscii(s[0..sp]);
            const right = trimAscii(s[sp + 1 ..]);
            if (std.mem.indexOfScalar(u8, right, ' ') != null) { return err.InvalidSize; }
            const w = LengthPercentageAuto.parse(left, false) ?? return err.InvalidSize;
            const h = LengthPercentageAuto.parse(right, false) ?? return err.InvalidSize;
            return BackgroundSize{ kind: BackgroundSizeKind.explicit, width: w, height: h };
        }
        const w = LengthPercentageAuto.parse(s, false) ?? return err.InvalidSize;
        return BackgroundSize{ kind: BackgroundSizeKind.explicit, width: w, height: LengthPercentageAuto.auto() };
    }

    fn toCss(buf: []u8): usize ! CssBackgroundError {
        if (_.kind == BackgroundSizeKind.cover) {
            const s = std.fmt.bufPrint(buf, "cover", .{}) catch return err.BufferTooSmall;
            return s.len;
        }
        if (_.kind == BackgroundSizeKind.contain) {
            const s = std.fmt.bufPrint(buf, "contain", .{}) catch return err.BufferTooSmall;
            return s.len;
        }
        let wbuf: [32]u8 = std.mem.zeroes([32]u8);
        const wlen = _.width.toCss(wbuf[0..])?;
        if (_.height.isAuto()) {
            const s = std.fmt.bufPrint(buf, "{s}", .{wbuf[0..wlen]}) catch return err.BufferTooSmall;
            return s.len;
        }
        let hbuf: [32]u8 = std.mem.zeroes([32]u8);
        const hlen = _.height.toCss(hbuf[0..])?;
        const s = std.fmt.bufPrint(buf, "{s} {s}", .{ wbuf[0..wlen], hbuf[0..hlen] }) catch return err.BufferTooSmall;
        return s.len;
    }
}

/// `background-attachment`（`scroll` / `fixed` / `local`）
pub enum BackgroundAttachment: u8 {
    scroll = 0,
    fixed = 1,
    local = 2,

    static fn default(): BackgroundAttachment { return .scroll; }

    static fn parse(input: string): BackgroundAttachment? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "scroll")) { return .scroll; }
        if (std.ascii.eqlIgnoreCase(s, "fixed")) { return .fixed; }
        if (std.ascii.eqlIgnoreCase(s, "local")) { return .local; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .scroll => "scroll",
            .fixed => "fixed",
            .local => "local",
        };
    }
}

/// `background-origin`（`padding-box` / `border-box` / `content-box`，默认 `padding-box`）
pub enum BackgroundOrigin: u8 {
    padding_box = 0,
    border_box = 1,
    content_box = 2,

    static fn default(): BackgroundOrigin { return .padding_box; }

    static fn parse(input: string): BackgroundOrigin? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "padding-box")) { return .padding_box; }
        if (std.ascii.eqlIgnoreCase(s, "border-box")) { return .border_box; }
        if (std.ascii.eqlIgnoreCase(s, "content-box")) { return .content_box; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .padding_box => "padding-box",
            .border_box => "border-box",
            .content_box => "content-box",
        };
    }

    fn intoClip(): BackgroundClip {
        return switch (_) {
            .padding_box => BackgroundClip.padding_box,
            .border_box => BackgroundClip.border_box,
            .content_box => BackgroundClip.content_box,
        };
    }
}

/// `background-clip`（`border-box` / `padding-box` / `content-box` / `text` / `border-area`）
pub enum BackgroundClip: u8 {
    border_box = 0,
    padding_box = 1,
    content_box = 2,
    text = 3,
    border_area = 4,

    static fn default(): BackgroundClip { return .border_box; }

    static fn parse(input: string): BackgroundClip? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "border-box")) { return .border_box; }
        if (std.ascii.eqlIgnoreCase(s, "padding-box")) { return .padding_box; }
        if (std.ascii.eqlIgnoreCase(s, "content-box")) { return .content_box; }
        if (std.ascii.eqlIgnoreCase(s, "text")) { return .text; }
        if (std.ascii.eqlIgnoreCase(s, "border-area") || std.ascii.eqlIgnoreCase(s, "border")) { return .border_area; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .border_box => "border-box",
            .padding_box => "padding-box",
            .content_box => "content-box",
            .text => "text",
            .border_area => "border-area",
        };
    }

    fn eqlOrigin(origin: BackgroundOrigin): bool {
        return switch (_) {
            .border_box => origin == BackgroundOrigin.border_box,
            .padding_box => origin == BackgroundOrigin.padding_box,
            .content_box => origin == BackgroundOrigin.content_box,
            else => false,
        };
    }

    fn isBackgroundBox(): bool {
        return _ == .border_box || _ == .padding_box || _ == .content_box;
    }
}

fn parsePosKeywordOrLen(tok: string): LengthPercentageAuto? {
    if (std.ascii.eqlIgnoreCase(tok, "left") || std.ascii.eqlIgnoreCase(tok, "top")) {
        return LengthPercentageAuto.percent(0.0);
    }
    if (std.ascii.eqlIgnoreCase(tok, "center")) {
        return LengthPercentageAuto.percent(50.0);
    }
    if (std.ascii.eqlIgnoreCase(tok, "right") || std.ascii.eqlIgnoreCase(tok, "bottom")) {
        return LengthPercentageAuto.percent(100.0);
    }
    if (lpa := LengthPercentageAuto.parse(tok, true)) {
        if (!lpa.isAuto()) {
            return lpa;
        }
    }
    return null;
}

/// `background-position`（双轴定位，默认 `0% 0%`）
pub struct BackgroundPosition {
    x: LengthPercentageAuto = LengthPercentageAuto.percent(0.0),
    y: LengthPercentageAuto = LengthPercentageAuto.percent(0.0),

    static fn default(): BackgroundPosition {
        return BackgroundPosition{};
    }

    fn isDefault(): bool {
        return _.x.isZero() && _.y.isZero();
    }

    static fn parse(input: string): BackgroundPosition ! CssBackgroundError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidPosition; }
        if (sp := std.mem.indexOfScalar(u8, s, ' ')) {
            const left = trimAscii(s[0..sp]);
            const right = trimAscii(s[sp + 1 ..]);
            if (std.mem.indexOfScalar(u8, right, ' ') != null) { return err.InvalidPosition; }
            // 支持垂直关键字在前（如 `top left` / `bottom center`）
            const left_is_vert = std.ascii.eqlIgnoreCase(left, "top") || std.ascii.eqlIgnoreCase(left, "bottom");
            const right_is_horiz = std.ascii.eqlIgnoreCase(right, "left") || std.ascii.eqlIgnoreCase(right, "right");
            if (left_is_vert && right_is_horiz) {
                const py = parsePosKeywordOrLen(left) ?? return err.InvalidPosition;
                const px = parsePosKeywordOrLen(right) ?? return err.InvalidPosition;
                return BackgroundPosition{ x: px, y: py };
            }
            const px = parsePosKeywordOrLen(left) ?? return err.InvalidPosition;
            const py = parsePosKeywordOrLen(right) ?? return err.InvalidPosition;
            return BackgroundPosition{ x: px, y: py };
        }
        if (std.ascii.eqlIgnoreCase(s, "top")) {
            return BackgroundPosition{ x: LengthPercentageAuto.percent(50.0), y: LengthPercentageAuto.percent(0.0) };
        }
        if (std.ascii.eqlIgnoreCase(s, "bottom")) {
            return BackgroundPosition{ x: LengthPercentageAuto.percent(50.0), y: LengthPercentageAuto.percent(100.0) };
        }
        const px = parsePosKeywordOrLen(s) ?? return err.InvalidPosition;
        return BackgroundPosition{ x: px, y: LengthPercentageAuto.percent(50.0) };
    }

    fn toCss(buf: []u8): usize ! CssBackgroundError {
        let xbuf: [32]u8 = std.mem.zeroes([32]u8);
        let ybuf: [32]u8 = std.mem.zeroes([32]u8);
        const xlen = _.x.toCss(xbuf[0..])?;
        const ylen = _.y.toCss(ybuf[0..])?;
        const res = std.fmt.bufPrint(buf, "{s} {s}", .{ xbuf[0..xlen], ybuf[0..ylen] }) catch return err.BufferTooSmall;
        return res.len;
    }
}

fn isImageToken(tok: string): bool {
    if (std.ascii.eqlIgnoreCase(tok, "none")) { return true; }
    if (tok.len > 4 && std.ascii.eqlIgnoreCase(tok[0..4], "url(") && tok[tok.len - 1] == ')') { return true; }
    if (tok.len > 16 && std.ascii.eqlIgnoreCase(tok[0..16], "linear-gradient(") && tok[tok.len - 1] == ')') { return true; }
    if (tok.len > 16 && std.ascii.eqlIgnoreCase(tok[0..16], "radial-gradient(") && tok[tok.len - 1] == ')') { return true; }
    return false;
}

fn isColorToken(tok: string): bool {
    if (tok.len >= 4 && tok[0] == '#') {
        return tok.len == 4 || tok.len == 5 || tok.len == 7 || tok.len == 9;
    }
    if (tok.len > 4 && (std.ascii.eqlIgnoreCase(tok[0..4], "rgb(") || std.ascii.eqlIgnoreCase(tok[0..4], "hsl(")) && tok[tok.len - 1] == ')') {
        return true;
    }
    if (tok.len > 5 && (std.ascii.eqlIgnoreCase(tok[0..5], "rgba(") || std.ascii.eqlIgnoreCase(tok[0..5], "hsla(")) && tok[tok.len - 1] == ')') {
        return true;
    }
    return std.ascii.eqlIgnoreCase(tok, "transparent")
        || std.ascii.eqlIgnoreCase(tok, "currentcolor")
        || std.ascii.eqlIgnoreCase(tok, "black")
        || std.ascii.eqlIgnoreCase(tok, "white")
        || std.ascii.eqlIgnoreCase(tok, "red")
        || std.ascii.eqlIgnoreCase(tok, "green")
        || std.ascii.eqlIgnoreCase(tok, "blue")
        || std.ascii.eqlIgnoreCase(tok, "gray");
}

fn tokenizeLayer(input: string, out: *[16]string): usize ! CssBackgroundError {
    const s = trimAscii(input);
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) {
            pos += 1;
        }
        if (pos >= s.len) { break; }
        if (s[pos] == '/') {
            if (count >= 16) { return err.InvalidBackground; }
            out.*[count] = "/";
            count += 1;
            pos += 1;
            continue;
        }
        const start = pos;
        let depth: usize = 0;
        while (pos < s.len) {
            const c = s[pos];
            if (c == '(') {
                depth += 1;
            } else if (c == ')') {
                if (depth == 0) { return err.InvalidBackground; }
                depth -= 1;
            } else if (depth == 0 && (c == ' ' || c == '\t' || c == '/')) {
                break;
            }
            pos += 1;
        }
        if (depth != 0) { return err.InvalidBackground; }
        if (count >= 16) { return err.InvalidBackground; }
        out.*[count] = s[start..pos];
        count += 1;
    }
    return count;
}

/// 单个 `background` 图层模型
pub struct BackgroundLayer {
    image: string = "none",
    position: BackgroundPosition = BackgroundPosition.default(),
    size: BackgroundSize = BackgroundSize.default(),
    repeat: BackgroundRepeat = BackgroundRepeat.default(),
    attachment: BackgroundAttachment = BackgroundAttachment.default(),
    origin: BackgroundOrigin = BackgroundOrigin.default(),
    clip: BackgroundClip = BackgroundClip.default(),
    color: string? = null,

    static fn default(): BackgroundLayer {
        return BackgroundLayer{};
    }

    fn isDefault(): bool {
        return std.ascii.eqlIgnoreCase(_.image, "none")
            && _.position.isDefault()
            && _.size.isDefault()
            && _.repeat.isDefault()
            && _.attachment == BackgroundAttachment.default()
            && _.origin == BackgroundOrigin.default()
            && _.clip == BackgroundClip.default()
            && _.color == null;
    }

    /// 解析单层 `background` 简写（支持 `position / size`、双 `<box>` origin/clip 规则与颜色仅限末层约束）
    static fn parse(input: string, allow_color: bool): BackgroundLayer ! CssBackgroundError {
        let toks: [16]string = ["", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""];
        const n = tokenizeLayer(input, &toks)?;
        if (n == 0) {
            return err.InvalidBackground;
        }

        let layer = BackgroundLayer{};
        let has_image = false;
        let has_pos = false;
        let has_size = false;
        let has_repeat = false;
        let has_attach = false;
        let box_count: usize = 0;
        let has_clip_only = false;

        let i: usize = 0;
        while (i < n) {
            const tok = toks[i];
            if (std.mem.eql(u8, tok, "/")) {
                // `/` 必须紧跟在 position 之后，且后接 1..2 个 size token
                if (!has_pos || has_size || i + 1 >= n) {
                    return err.InvalidBackground;
                }
                i += 1;
                if (std.ascii.eqlIgnoreCase(toks[i], "cover")) {
                    layer.size = BackgroundSize.cover();
                    has_size = true;
                    i += 1;
                } else if (std.ascii.eqlIgnoreCase(toks[i], "contain")) {
                    layer.size = BackgroundSize.contain();
                    has_size = true;
                    i += 1;
                } else {
                    const w = LengthPercentageAuto.parse(toks[i], false) ?? return err.InvalidSize;
                    i += 1;
                    let h = LengthPercentageAuto.auto();
                    if (i < n) {
                        if (h_parsed := LengthPercentageAuto.parse(toks[i], false)) {
                            h = h_parsed;
                            i += 1;
                        }
                    }
                    layer.size = BackgroundSize{ kind: BackgroundSizeKind.explicit, width: w, height: h };
                    has_size = true;
                }
                continue;
            }

            if (!has_image && isImageToken(tok)) {
                layer.image = tok;
                has_image = true;
                i += 1;
                continue;
            }

            if (!has_repeat && (std.ascii.eqlIgnoreCase(tok, "repeat-x") || std.ascii.eqlIgnoreCase(tok, "repeat-y"))) {
                layer.repeat = BackgroundRepeat.parse(tok)?;
                has_repeat = true;
                i += 1;
                continue;
            }
            if (!has_repeat) {
                if (rx := BackgroundRepeatKeyword.parse(tok)) {
                    i += 1;
                    let ry = rx;
                    if (i < n) {
                        if (ry_parsed := BackgroundRepeatKeyword.parse(toks[i])) {
                            ry = ry_parsed;
                            i += 1;
                        }
                    }
                    layer.repeat = BackgroundRepeat{ x: rx, y: ry };
                    has_repeat = true;
                    continue;
                }
            }

            if (!has_attach) {
                if (att := BackgroundAttachment.parse(tok)) {
                    layer.attachment = att;
                    has_attach = true;
                    i += 1;
                    continue;
                }
            }

            if (orig := BackgroundOrigin.parse(tok)) {
                if (box_count == 0) {
                    // 单个 <box> 同时设定 origin 与 clip
                    layer.origin = orig;
                    if (!has_clip_only) {
                        layer.clip = orig.intoClip();
                    }
                    box_count = 1;
                } else if (box_count == 1 && !has_clip_only) {
                    // 第二个 <box> 设定 clip
                    layer.clip = orig.intoClip();
                    box_count = 2;
                } else {
                    return err.InvalidBackground;
                }
                i += 1;
                continue;
            }

            if (!has_clip_only) {
                if (cl := BackgroundClip.parse(tok)) {
                    if (!cl.isBackgroundBox()) {
                        layer.clip = cl;
                        has_clip_only = true;
                        i += 1;
                        continue;
                    }
                }
            }

            if (!has_pos && parsePosKeywordOrLen(tok) != null) {
                let pbuf: [64]u8 = std.mem.zeroes([64]u8);
                let pslice: string = tok;
                if (i + 1 < n && !std.mem.eql(u8, toks[i + 1], "/") && parsePosKeywordOrLen(toks[i + 1]) != null) {
                    const combined = std.fmt.bufPrint(pbuf[0..], "{s} {s}", .{ tok, toks[i + 1] }) catch return err.BufferTooSmall;
                    pslice = combined;
                    i += 2;
                } else {
                    i += 1;
                }
                layer.position = BackgroundPosition.parse(pslice)?;
                has_pos = true;
                continue;
            }

            if (allow_color && layer.color == null && isColorToken(tok)) {
                layer.color = tok;
                i += 1;
                continue;
            }

            return err.InvalidBackground;
        }

        return layer;
    }

    /// 规范化序列化单层 `background`（省略默认值，保留 `position / size` 与双框简写规则）
    fn toCss(buf: []u8): usize ! CssBackgroundError {
        let pos: usize = 0;
        let wrote_any = false;

        if (c := _.color) {
            const s = std.fmt.bufPrint(buf[pos..], "{s}", .{c}) catch return err.BufferTooSmall;
            pos += s.len;
            wrote_any = true;
        }

        if (!std.ascii.eqlIgnoreCase(_.image, "none")) {
            if (wrote_any) {
                if (pos >= buf.len) { return err.BufferTooSmall; }
                buf[pos] = ' ';
                pos += 1;
            }
            const s = std.fmt.bufPrint(buf[pos..], "{s}", .{_.image}) catch return err.BufferTooSmall;
            pos += s.len;
            wrote_any = true;
        }

        if (!_.position.isDefault() || !_.size.isDefault()) {
            if (wrote_any) {
                if (pos >= buf.len) { return err.BufferTooSmall; }
                buf[pos] = ' ';
                pos += 1;
            }
            pos += _.position.toCss(buf[pos..])?;
            if (!_.size.isDefault()) {
                const sep = " / ";
                if (pos + sep.len > buf.len) { return err.BufferTooSmall; }
                @memcpy(buf[pos .. pos + sep.len], sep);
                pos += sep.len;
                pos += _.size.toCss(buf[pos..])?;
            }
            wrote_any = true;
        }

        if (!_.repeat.isDefault()) {
            if (wrote_any) {
                if (pos >= buf.len) { return err.BufferTooSmall; }
                buf[pos] = ' ';
                pos += 1;
            }
            pos += _.repeat.toCss(buf[pos..])?;
            wrote_any = true;
        }

        if (_.attachment != BackgroundAttachment.default()) {
            if (wrote_any) {
                if (pos >= buf.len) { return err.BufferTooSmall; }
                buf[pos] = ' ';
                pos += 1;
            }
            const s = std.fmt.bufPrint(buf[pos..], "{s}", .{_.attachment.toCss()}) catch return err.BufferTooSmall;
            pos += s.len;
            wrote_any = true;
        }

        const is_def_boxes = _.origin == BackgroundOrigin.padding_box && _.clip == BackgroundClip.border_box;
        if (!is_def_boxes) {
            if (wrote_any) {
                if (pos >= buf.len) { return err.BufferTooSmall; }
                buf[pos] = ' ';
                pos += 1;
            }
            if (_.clip.eqlOrigin(_.origin)) {
                const s = std.fmt.bufPrint(buf[pos..], "{s}", .{_.origin.toCss()}) catch return err.BufferTooSmall;
                pos += s.len;
            } else {
                const s = std.fmt.bufPrint(buf[pos..], "{s} {s}", .{ _.origin.toCss(), _.clip.toCss() }) catch return err.BufferTooSmall;
                pos += s.len;
            }
            wrote_any = true;
        }

        if (!wrote_any) {
            const s = std.fmt.bufPrint(buf[pos..], "none", .{}) catch return err.BufferTooSmall;
            pos += s.len;
        }
        return pos;
    }
}

/// 多图层 `background` 简写（逗号分隔，最多 8 层，零堆分配）
pub struct Background {
    layers: [8]BackgroundLayer = [
        BackgroundLayer{},
        BackgroundLayer{},
        BackgroundLayer{},
        BackgroundLayer{},
        BackgroundLayer{},
        BackgroundLayer{},
        BackgroundLayer{},
        BackgroundLayer{},
    ],
    len: usize = 0,

    static fn parse(input: string): Background ! CssBackgroundError {
        const s = trimAscii(input);
        if (s.len == 0) {
            return err.InvalidBackground;
        }

        // 先按顶层逗号切分（忽略括号内部逗号，如 linear-gradient(...) / rgb(...)）
        let raw_layers: [8]string = ["", "", "", "", "", "", "", ""];
        let count: usize = 0;
        let start: usize = 0;
        let depth: usize = 0;
        let pos: usize = 0;
        while (pos <= s.len) {
            const at_end = pos == s.len;
            if (!at_end) {
                if (s[pos] == '(') { depth += 1; }
                else if (s[pos] == ')') {
                    if (depth == 0) { return err.InvalidBackground; }
                    depth -= 1;
                }
            }
            if (at_end || (depth == 0 && s[pos] == ',')) {
                const part = trimAscii(s[start..pos]);
                if (part.len == 0) { return err.InvalidBackground; }
                if (count >= 8) { return err.TooManyLayers; }
                raw_layers[count] = part;
                count += 1;
                start = pos + 1;
            }
            pos += 1;
        }
        if (depth != 0) { return err.InvalidBackground; }

        let bg = Background{ len: count };
        let i: usize = 0;
        while (i < count) {
            const is_last = (i + 1 == count);
            bg.layers[i] = BackgroundLayer.parse(raw_layers[i], is_last)?;
            i += 1;
        }
        return bg;
    }

    fn toCss(buf: []u8): usize ! CssBackgroundError {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                const sep = ", ";
                if (pos + sep.len > buf.len) { return err.BufferTooSmall; }
                @memcpy(buf[pos .. pos + sep.len], sep);
                pos += sep.len;
            }
            pos += _.layers[i].toCss(buf[pos..])?;
            i += 1;
        }
        return pos;
    }
}
