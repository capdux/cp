// Source: Absorbed from /data/dev/bun/src/css/properties/align.cp | Synced: 2026-11-23T21:32:52Z
// Pure-CP zero-allocation W3C CSS Box Alignment Level 3 parser, shorthand expander & serializer.
// Supports:
// - `<overflow-position>` (`safe` / `unsafe`) & `<baseline-position>` (`[first | last]? baseline`)
// - `<content-distribution>` (`space-between` / `space-around` / `space-evenly` / `stretch`)
// - `<content-position>` / `<self-position>` & `legacy [left | right | center]`
// - Full longhands (`align-content`, `justify-content`, `align-items`, `justify-items`, `align-self`, `justify-self`)
// - Shorthands (`place-content`, `place-items`, `place-self`) with W3C default fallback & minimal serialization

/// CSS Box Alignment Level 3 错误集
pub err CssBoxAlignError {
    EmptyInput,
    InvalidKeyword,
    InvalidSyntax,
    TrailingTokens,
    BufferTooSmall,
}

/// `<overflow-position>` 溢出对齐修饰符（`safe` / `unsafe`）
pub enum CssOverflowPosition: u8 {
    Safe = 0,
    Unsafe = 1,

    static fn parse(raw: []const u8): CssOverflowPosition? {
        if (eqlAsciiIgnoreCase(raw, "safe")) return .Safe;
        if (eqlAsciiIgnoreCase(raw, "unsafe")) return .Unsafe;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Safe => "safe",
            .Unsafe => "unsafe",
        };
    }
}

/// `<baseline-position>` 基线对齐位置（`baseline` / `first baseline` / `last baseline`）
pub enum CssBaselinePosition: u8 {
    First = 0,
    Last = 1,

    fn asStr(): []const u8 {
        return switch (_) {
            .First => "baseline",
            .Last => "last baseline",
        };
    }
}

/// `<content-distribution>` 内容分布关键字
pub enum CssContentDistribution: u8 {
    SpaceBetween = 0,
    SpaceAround = 1,
    SpaceEvenly = 2,
    Stretch = 3,

    static fn parse(raw: []const u8): CssContentDistribution? {
        if (eqlAsciiIgnoreCase(raw, "space-between")) return .SpaceBetween;
        if (eqlAsciiIgnoreCase(raw, "space-around")) return .SpaceAround;
        if (eqlAsciiIgnoreCase(raw, "space-evenly")) return .SpaceEvenly;
        if (eqlAsciiIgnoreCase(raw, "stretch")) return .Stretch;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .SpaceBetween => "space-between",
            .SpaceAround => "space-around",
            .SpaceEvenly => "space-evenly",
            .Stretch => "stretch",
        };
    }
}

/// `<content-position>` 内容位置关键字
pub enum CssContentPosition: u8 {
    Center = 0,
    Start = 1,
    End = 2,
    FlexStart = 3,
    FlexEnd = 4,
    Left = 5,
    Right = 6,

    static fn parse(raw: []const u8, allow_left_right: bool): CssContentPosition? {
        if (eqlAsciiIgnoreCase(raw, "center")) return .Center;
        if (eqlAsciiIgnoreCase(raw, "start")) return .Start;
        if (eqlAsciiIgnoreCase(raw, "end")) return .End;
        if (eqlAsciiIgnoreCase(raw, "flex-start")) return .FlexStart;
        if (eqlAsciiIgnoreCase(raw, "flex-end")) return .FlexEnd;
        if (allow_left_right) {
            if (eqlAsciiIgnoreCase(raw, "left")) return .Left;
            if (eqlAsciiIgnoreCase(raw, "right")) return .Right;
        }
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Center => "center",
            .Start => "start",
            .End => "end",
            .FlexStart => "flex-start",
            .FlexEnd => "flex-end",
            .Left => "left",
            .Right => "right",
        };
    }
}

/// `<self-position>` 自身/子项对齐位置关键字
pub enum CssSelfPosition: u8 {
    Center = 0,
    Start = 1,
    End = 2,
    SelfStart = 3,
    SelfEnd = 4,
    FlexStart = 5,
    FlexEnd = 6,
    Left = 7,
    Right = 8,

    static fn parse(raw: []const u8, allow_left_right: bool): CssSelfPosition? {
        if (eqlAsciiIgnoreCase(raw, "center")) return .Center;
        if (eqlAsciiIgnoreCase(raw, "start")) return .Start;
        if (eqlAsciiIgnoreCase(raw, "end")) return .End;
        if (eqlAsciiIgnoreCase(raw, "self-start")) return .SelfStart;
        if (eqlAsciiIgnoreCase(raw, "self-end")) return .SelfEnd;
        if (eqlAsciiIgnoreCase(raw, "flex-start")) return .FlexStart;
        if (eqlAsciiIgnoreCase(raw, "flex-end")) return .FlexEnd;
        if (allow_left_right) {
            if (eqlAsciiIgnoreCase(raw, "left")) return .Left;
            if (eqlAsciiIgnoreCase(raw, "right")) return .Right;
        }
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Center => "center",
            .Start => "start",
            .End => "end",
            .SelfStart => "self-start",
            .SelfEnd => "self-end",
            .FlexStart => "flex-start",
            .FlexEnd => "flex-end",
            .Left => "left",
            .Right => "right",
        };
    }
}

/// `justify-items` 兼容旧版对齐关键字（`legacy [left | right | center]`）
pub enum CssLegacyJustify: u8 {
    LegacyOnly = 0,
    Left = 1,
    Right = 2,
    Center = 3,

    fn asStr(): []const u8 {
        return switch (_) {
            .LegacyOnly => "legacy",
            .Left => "legacy left",
            .Right => "legacy right",
            .Center => "legacy center",
        };
    }
}

/// `align-content` / `justify-content` 值的类别
pub enum CssContentAlignKind: u8 {
    Normal = 0,
    Baseline = 1,
    Distribution = 2,
    Position = 3,
}

/// `align-content` 与 `justify-content` 统一表示
pub struct CssBoxContentAlign {
    kind: CssContentAlignKind = .Normal,
    baseline: CssBaselinePosition = .First,
    distribution: CssContentDistribution = .Stretch,
    overflow: CssOverflowPosition? = null,
    position: CssContentPosition = .Start,

    static fn parseAlignContent(input: []const u8): CssBoxContentAlign! {
        let cur = TokenCursor.init(input);
        const val = parseContentAlignFromCursor(&cur, false)?;
        if (cur.next() != null) return err.TrailingTokens;
        return val;
    }

    static fn parseJustifyContent(input: []const u8): CssBoxContentAlign! {
        let cur = TokenCursor.init(input);
        const val = parseContentAlignFromCursor(&cur, true)?;
        if (cur.next() != null) return err.TrailingTokens;
        return val;
    }

    fn eql(other: CssBoxContentAlign): bool {
        if (_.kind != other.kind) return false;
        return switch (_.kind) {
            .Normal => true,
            .Baseline => _.baseline == other.baseline,
            .Distribution => _.distribution == other.distribution,
            .Position => _.position == other.position && optOverflowEql(_.overflow, other.overflow),
        };
    }

    fn writeCss(out: []u8, offset: usize): usize! {
        return switch (_.kind) {
            .Normal => writeSlice(out, offset, "normal"),
            .Baseline => writeSlice(out, offset, _.baseline.asStr()),
            .Distribution => writeSlice(out, offset, _.distribution.asStr()),
            .Position => {
                let pos = offset;
                if (ov := _.overflow) {
                    pos = writeSlice(out, pos, ov.asStr())?;
                    pos = writeSlice(out, pos, " ")?;
                }
                return writeSlice(out, pos, _.position.asStr());
            },
        };
    }

    fn toCss(out: []u8): usize! {
        return _.writeCss(out, 0);
    }
}

/// `align-items` / `justify-items` / `align-self` / `justify-self` 值的类别
pub enum CssItemAlignKind: u8 {
    Auto = 0,
    Normal = 1,
    Stretch = 2,
    Baseline = 3,
    SelfPosition = 4,
    Legacy = 5,
}

/// `align-items` / `justify-items` / `align-self` / `justify-self` 统一表示
pub struct CssBoxItemAlign {
    kind: CssItemAlignKind = .Normal,
    baseline: CssBaselinePosition = .First,
    overflow: CssOverflowPosition? = null,
    self_pos: CssSelfPosition = .Start,
    legacy: CssLegacyJustify = .LegacyOnly,

    static fn parseAlignItems(input: []const u8): CssBoxItemAlign! {
        let cur = TokenCursor.init(input);
        const val = parseItemAlignFromCursor(&cur, false, false, false)?;
        if (cur.next() != null) return err.TrailingTokens;
        return val;
    }

    static fn parseJustifyItems(input: []const u8): CssBoxItemAlign! {
        let cur = TokenCursor.init(input);
        const val = parseItemAlignFromCursor(&cur, false, true, true)?;
        if (cur.next() != null) return err.TrailingTokens;
        return val;
    }

    static fn parseAlignSelf(input: []const u8): CssBoxItemAlign! {
        let cur = TokenCursor.init(input);
        const val = parseItemAlignFromCursor(&cur, true, false, false)?;
        if (cur.next() != null) return err.TrailingTokens;
        return val;
    }

    static fn parseJustifySelf(input: []const u8): CssBoxItemAlign! {
        let cur = TokenCursor.init(input);
        const val = parseItemAlignFromCursor(&cur, true, true, false)?;
        if (cur.next() != null) return err.TrailingTokens;
        return val;
    }

    fn eql(other: CssBoxItemAlign): bool {
        if (_.kind != other.kind) return false;
        return switch (_.kind) {
            .Auto, .Normal, .Stretch => true,
            .Baseline => _.baseline == other.baseline,
            .SelfPosition => _.self_pos == other.self_pos && optOverflowEql(_.overflow, other.overflow),
            .Legacy => _.legacy == other.legacy,
        };
    }

    fn writeCss(out: []u8, offset: usize): usize! {
        return switch (_.kind) {
            .Auto => writeSlice(out, offset, "auto"),
            .Normal => writeSlice(out, offset, "normal"),
            .Stretch => writeSlice(out, offset, "stretch"),
            .Baseline => writeSlice(out, offset, _.baseline.asStr()),
            .SelfPosition => {
                let pos = offset;
                if (ov := _.overflow) {
                    pos = writeSlice(out, pos, ov.asStr())?;
                    pos = writeSlice(out, pos, " ")?;
                }
                return writeSlice(out, pos, _.self_pos.asStr());
            },
            .Legacy => writeSlice(out, offset, _.legacy.asStr()),
        };
    }

    fn toCss(out: []u8): usize! {
        return _.writeCss(out, 0);
    }
}

/// W3C `place-content` 简写属性（`align-content` + `justify-content`）
pub struct CssPlaceContent {
    align_val: CssBoxContentAlign = CssBoxContentAlign{},
    justify_val: CssBoxContentAlign = CssBoxContentAlign{},

    /// 解析 `place-content`：单值时若为 `baseline`，`justify-content` 按 W3C 规范回退为 `start`
    static fn parse(input: []const u8): CssPlaceContent! {
        let cur = TokenCursor.init(input);
        const a = parseContentAlignFromCursor(&cur, false)?;
        const saved = cur.pos;
        if (cur.peek() == null) {
            const j = if (a.kind == .Baseline)
                CssBoxContentAlign{ kind: .Position, position: .Start }
            else
                a;
            return CssPlaceContent{ align_val: a, justify_val: j };
        }
        const j = parseContentAlignFromCursor(&cur, true)?;
        if (cur.next() != null) return err.TrailingTokens;
        * = saved;
        return CssPlaceContent{ align_val: a, justify_val: j };
    }

    fn eql(other: CssPlaceContent): bool {
        return _.align_val.eql(other.align_val) && _.justify_val.eql(other.justify_val);
    }

    /// 最小化 CSS 序列化：当 `align` 与 `justify` 相等时折叠为单值
    fn toCss(out: []u8): usize! {
        let pos = _.align_val.writeCss(out, 0)?;
        if (!_.align_val.eql(_.justify_val)) {
            pos = writeSlice(out, pos, " ")?;
            pos = _.justify_val.writeCss(out, pos)?;
        }
        return pos;
    }
}

/// W3C `place-items` 简写属性（`align-items` + `justify-items`）
pub struct CssPlaceItems {
    align_val: CssBoxItemAlign = CssBoxItemAlign{},
    justify_val: CssBoxItemAlign = CssBoxItemAlign{},

    static fn parse(input: []const u8): CssPlaceItems! {
        let cur = TokenCursor.init(input);
        const a = parseItemAlignFromCursor(&cur, false, false, false)?;
        if (cur.peek() == null) {
            return CssPlaceItems{ align_val: a, justify_val: a };
        }
        const j = parseItemAlignFromCursor(&cur, false, true, true)?;
        if (cur.next() != null) return err.TrailingTokens;
        return CssPlaceItems{ align_val: a, justify_val: j };
    }

    fn eql(other: CssPlaceItems): bool {
        return _.align_val.eql(other.align_val) && _.justify_val.eql(other.justify_val);
    }

    fn toCss(out: []u8): usize! {
        let pos = _.align_val.writeCss(out, 0)?;
        if (!_.align_val.eql(_.justify_val)) {
            pos = writeSlice(out, pos, " ")?;
            pos = _.justify_val.writeCss(out, pos)?;
        }
        return pos;
    }
}

/// W3C `place-self` 简写属性（`align-self` + `justify-self`）
pub struct CssPlaceSelf {
    align_val: CssBoxItemAlign = CssBoxItemAlign{ kind: .Auto },
    justify_val: CssBoxItemAlign = CssBoxItemAlign{ kind: .Auto },

    static fn parse(input: []const u8): CssPlaceSelf! {
        let cur = TokenCursor.init(input);
        const a = parseItemAlignFromCursor(&cur, true, false, false)?;
        if (cur.peek() == null) {
            return CssPlaceSelf{ align_val: a, justify_val: a };
        }
        const j = parseItemAlignFromCursor(&cur, true, true, false)?;
        if (cur.next() != null) return err.TrailingTokens;
        return CssPlaceSelf{ align_val: a, justify_val: j };
    }

    fn eql(other: CssPlaceSelf): bool {
        return _.align_val.eql(other.align_val) && _.justify_val.eql(other.justify_val);
    }

    fn toCss(out: []u8): usize! {
        let pos = _.align_val.writeCss(out, 0)?;
        if (!_.align_val.eql(_.justify_val)) {
            pos = writeSlice(out, pos, " ")?;
            pos = _.justify_val.writeCss(out, pos)?;
        }
        return pos;
    }
}

struct TokenCursor {
    src: []const u8,
    pos: usize = 0,

    static fn init(src: []const u8): TokenCursor {
        return TokenCursor{ src: src, pos: 0 };
    }

    fn skipWs(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn peek(*_): ([]const u8)? {
        const saved = _.pos;
        const tok = _.next();
        _.pos = saved;
        return tok;
    }

    fn next(*_): ([]const u8)? {
        _.skipWs();
        if (_.pos >= _.src.len) return null;
        const start = _.pos;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                break;
            }
            _.pos += 1;
        }
        return _.src[start.._.pos];
    }
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn optOverflowEql(a: CssOverflowPosition?, b: CssOverflowPosition?): bool {
    if (va := a) {
        if (vb := b) return va == vb;
        return false;
    }
    return b == null;
}

fn writeSlice(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) {
        return err.BufferTooSmall;
    }
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn parseBaselineFromFirst(first: []const u8, cur: *TokenCursor): CssBaselinePosition?! {
    if (eqlAsciiIgnoreCase(first, "baseline")) {
        return CssBaselinePosition.First;
    }
    if (eqlAsciiIgnoreCase(first, "first")) {
        const next_tok = cur.next() ?? return err.InvalidSyntax;
        if (!eqlAsciiIgnoreCase(next_tok, "baseline")) return err.InvalidSyntax;
        return CssBaselinePosition.First;
    }
    if (eqlAsciiIgnoreCase(first, "last")) {
        const next_tok = cur.next() ?? return err.InvalidSyntax;
        if (!eqlAsciiIgnoreCase(next_tok, "baseline")) return err.InvalidSyntax;
        return CssBaselinePosition.Last;
    }
    return null;
}

fn parseContentAlignFromCursor(cur: *TokenCursor, is_justify: bool): CssBoxContentAlign! {
    const first = cur.next() ?? return err.EmptyInput;

    if (eqlAsciiIgnoreCase(first, "normal")) {
        return CssBoxContentAlign{ kind: .Normal };
    }

    if (!is_justify) {
        if (bl := parseBaselineFromFirst(first, cur)?) {
            return CssBoxContentAlign{ kind: .Baseline, baseline: bl };
        }
    }

    if (dist := CssContentDistribution.parse(first)) {
        return CssBoxContentAlign{ kind: .Distribution, distribution: dist };
    }

    if (ov := CssOverflowPosition.parse(first)) {
        const pos_tok = cur.next() ?? return err.InvalidSyntax;
        const cp = CssContentPosition.parse(pos_tok, is_justify) ?? return err.InvalidKeyword;
        return CssBoxContentAlign{
            kind: .Position,
            overflow: ov,
            position: cp,
        };
    }

    if (cp := CssContentPosition.parse(first, is_justify)) {
        return CssBoxContentAlign{
            kind: .Position,
            overflow: null,
            position: cp,
        };
    }

    return err.InvalidKeyword;
}

fn parseLegacyDir(tok: []const u8): CssLegacyJustify? {
    if (eqlAsciiIgnoreCase(tok, "left")) return .Left;
    if (eqlAsciiIgnoreCase(tok, "right")) return .Right;
    if (eqlAsciiIgnoreCase(tok, "center")) return .Center;
    return null;
}

fn parseItemAlignFromCursor(
    cur: *TokenCursor,
    allow_auto: bool,
    allow_left_right: bool,
    allow_legacy: bool,
): CssBoxItemAlign! {
    const first = cur.next() ?? return err.EmptyInput;

    if (allow_auto && eqlAsciiIgnoreCase(first, "auto")) {
        return CssBoxItemAlign{ kind: .Auto };
    }
    if (eqlAsciiIgnoreCase(first, "normal")) {
        return CssBoxItemAlign{ kind: .Normal };
    }
    if (eqlAsciiIgnoreCase(first, "stretch")) {
        return CssBoxItemAlign{ kind: .Stretch };
    }

    if (bl := parseBaselineFromFirst(first, cur)?) {
        return CssBoxItemAlign{ kind: .Baseline, baseline: bl };
    }

    if (allow_legacy && eqlAsciiIgnoreCase(first, "legacy")) {
        if (next_peek := cur.peek()) {
            if (dir := parseLegacyDir(next_peek)) {
                * = cur.next();
                return CssBoxItemAlign{ kind: .Legacy, legacy: dir };
            }
        }
        return CssBoxItemAlign{ kind: .Legacy, legacy: .LegacyOnly };
    }

    if (ov := CssOverflowPosition.parse(first)) {
        const pos_tok = cur.next() ?? return err.InvalidSyntax;
        const sp = CssSelfPosition.parse(pos_tok, allow_left_right) ?? return err.InvalidKeyword;
        return CssBoxItemAlign{
            kind: .SelfPosition,
            overflow: ov,
            self_pos: sp,
        };
    }

    if (allow_legacy) {
        if (dir := parseLegacyDir(first)) {
            if (next_peek := cur.peek()) {
                if (eqlAsciiIgnoreCase(next_peek, "legacy")) {
                    * = cur.next();
                    return CssBoxItemAlign{ kind: .Legacy, legacy: dir };
                }
            }
        }
    }

    if (sp := CssSelfPosition.parse(first, allow_left_right)) {
        return CssBoxItemAlign{
            kind: .SelfPosition,
            overflow: null,
            self_pos: sp,
        };
    }

    return err.InvalidKeyword;
}
