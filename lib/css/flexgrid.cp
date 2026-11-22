// Source: Absorbed from /data/dev/bun/src/css/properties/flex.cp | Synced: 2026-11-22T08:18:56Z

/// CSS Flexbox / Box Alignment / Grid 布局属性解析错误集
pub err CssLayoutError {
    InvalidFlexFlow,
    InvalidFlexShorthand,
    InvalidAlignment,
    InvalidGridAutoFlow,
    InvalidTrackBreadth,
    BufferTooSmall,
}

fn trimAscii(s: string): string {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn splitWhitespaceTokens(input: string, out: *[4]string): usize {
    const s = trimAscii(input);
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) {
            pos += 1;
        }
        if (pos >= s.len) {
            break;
        }
        const start = pos;
        while (pos < s.len && s[pos] != ' ' && s[pos] != '\t') {
            pos += 1;
        }
        if (count < 4) {
            out.*[count] = s[start..pos];
        }
        count += 1;
    }
    return count;
}

/// CSS `flex-direction`（4 变体）
pub enum FlexDirection: u8 {
    row = 0,
    row_reverse = 1,
    column = 2,
    column_reverse = 3,

    static fn default(): FlexDirection {
        return .row;
    }

    static fn parse(input: string): FlexDirection? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "row")) { return .row; }
        if (std.ascii.eqlIgnoreCase(s, "row-reverse")) { return .row_reverse; }
        if (std.ascii.eqlIgnoreCase(s, "column")) { return .column; }
        if (std.ascii.eqlIgnoreCase(s, "column-reverse")) { return .column_reverse; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .row => "row",
            .row_reverse => "row-reverse",
            .column => "column",
            .column_reverse => "column-reverse",
        };
    }

    fn isHorizontal(): bool {
        return _ == .row || _ == .row_reverse;
    }

    fn isVertical(): bool {
        return _ == .column || _ == .column_reverse;
    }

    fn isReversed(): bool {
        return _ == .row_reverse || _ == .column_reverse;
    }
}

/// CSS `flex-wrap`（3 变体）
pub enum FlexWrap: u8 {
    nowrap = 0,
    wrap = 1,
    wrap_reverse = 2,

    static fn default(): FlexWrap {
        return .nowrap;
    }

    static fn parse(input: string): FlexWrap? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "nowrap")) { return .nowrap; }
        if (std.ascii.eqlIgnoreCase(s, "wrap")) { return .wrap; }
        if (std.ascii.eqlIgnoreCase(s, "wrap-reverse")) { return .wrap_reverse; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .nowrap => "nowrap",
            .wrap => "wrap",
            .wrap_reverse => "wrap-reverse",
        };
    }

    fn isMultiLine(): bool {
        return _ != .nowrap;
    }
}

/// CSS `flex-flow` 简写（`<flex-direction> || <flex-wrap>`）
pub struct FlexFlow {
    direction: FlexDirection = FlexDirection.row,
    wrap: FlexWrap = FlexWrap.nowrap,

    static fn default(): FlexFlow {
        return FlexFlow{};
    }

    static fn parse(input: string): FlexFlow ! CssLayoutError {
        let toks: [4]string = ["", "", "", ""];
        const n = splitWhitespaceTokens(input, &toks);
        if (n == 0 || n > 2) {
            return err.InvalidFlexFlow;
        }
        let dir: FlexDirection? = null;
        let wrp: FlexWrap? = null;
        let i: usize = 0;
        while (i < n) {
            if (d := FlexDirection.parse(toks[i])) {
                if (dir != null) { return err.InvalidFlexFlow; }
                dir = d;
            } else if (w := FlexWrap.parse(toks[i])) {
                if (wrp != null) { return err.InvalidFlexFlow; }
                wrp = w;
            } else {
                return err.InvalidFlexFlow;
            }
            i += 1;
        }
        return FlexFlow{
            direction: dir ?? FlexDirection.default(),
            wrap: wrp ?? FlexWrap.default(),
        };
    }

    fn toCss(buf: []u8): usize ! CssLayoutError {
        const is_def_dir = _.direction == FlexDirection.row;
        const is_def_wrap = _.wrap == FlexWrap.nowrap;
        const formatted = if (is_def_dir && is_def_wrap)
            std.fmt.bufPrint(buf, "row", .{})
        else if (is_def_dir)
            std.fmt.bufPrint(buf, "{s}", .{_.wrap.toCss()})
        else if (is_def_wrap)
            std.fmt.bufPrint(buf, "{s}", .{_.direction.toCss()})
        else
            std.fmt.bufPrint(buf, "{s} {s}", .{ _.direction.toCss(), _.wrap.toCss() });
        const slice = formatted catch return err.BufferTooSmall;
        return slice.len;
    }
}

/// CSS `flex-basis` 类型
pub enum FlexBasisKind: u8 {
    auto = 0,
    px = 1,
    percent = 2,
}

/// CSS `flex-basis` 值
pub struct FlexBasis {
    kind: FlexBasisKind = FlexBasisKind.auto,
    value: f64 = 0.0,

    static fn auto(): FlexBasis {
        return FlexBasis{ kind: FlexBasisKind.auto, value: 0.0 };
    }

    static fn px(v: f64): FlexBasis {
        return FlexBasis{ kind: FlexBasisKind.px, value: v };
    }

    static fn percent(v: f64): FlexBasis {
        return FlexBasis{ kind: FlexBasisKind.percent, value: v };
    }

    static fn parse(input: string): FlexBasis? {
        const s = trimAscii(input);
        if (s.len == 0) { return null; }
        if (std.ascii.eqlIgnoreCase(s, "auto")) { return FlexBasis.auto(); }
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "px")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 2]) catch return null;
            if (std.math.isNan(v) || v < 0.0) { return null; }
            return FlexBasis.px(v);
        }
        if (s.len > 1 && s[s.len - 1] == '%') {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 1]) catch return null;
            if (std.math.isNan(v) || v < 0.0) { return null; }
            return FlexBasis.percent(v);
        }
        return null;
    }

    fn toCss(buf: []u8): usize ! CssLayoutError {
        const res = switch (_.kind) {
            .auto => std.fmt.bufPrint(buf, "auto", .{}),
            .px => std.fmt.bufPrint(buf, "{d}px", .{_.value}),
            .percent => std.fmt.bufPrint(buf, "{d}%", .{_.value}),
        } catch return err.BufferTooSmall;
        return res.len;
    }
}

fn parseNonNegNumber(tok: string): f64? {
    const v = std.fmt.parseFloat(f64, tok) catch return null;
    if (std.math.isNan(v) || std.math.isInf(v) || v < 0.0) {
        return null;
    }
    return v;
}

/// CSS `flex` 简写（`none` / `auto` / `initial` / `<grow> <shrink>? <basis>?`）
pub struct FlexShorthand {
    grow: f64 = 0.0,
    shrink: f64 = 1.0,
    basis: FlexBasis = FlexBasis.auto(),

    static fn initial(): FlexShorthand {
        return FlexShorthand{ grow: 0.0, shrink: 1.0, basis: FlexBasis.auto() };
    }

    static fn auto(): FlexShorthand {
        return FlexShorthand{ grow: 1.0, shrink: 1.0, basis: FlexBasis.auto() };
    }

    static fn none(): FlexShorthand {
        return FlexShorthand{ grow: 0.0, shrink: 0.0, basis: FlexBasis.auto() };
    }

    static fn parse(input: string): FlexShorthand ! CssLayoutError {
        let toks: [4]string = ["", "", "", ""];
        const n = splitWhitespaceTokens(input, &toks);
        if (n == 0 || n > 3) {
            return err.InvalidFlexShorthand;
        }
        if (n == 1) {
            if (std.ascii.eqlIgnoreCase(toks[0], "none")) { return FlexShorthand.none(); }
            if (std.ascii.eqlIgnoreCase(toks[0], "auto")) { return FlexShorthand.auto(); }
            if (std.ascii.eqlIgnoreCase(toks[0], "initial")) { return FlexShorthand.initial(); }
            if (g := parseNonNegNumber(toks[0])) {
                return FlexShorthand{ grow: g, shrink: 1.0, basis: FlexBasis.percent(0.0) };
            }
            if (b := FlexBasis.parse(toks[0])) {
                return FlexShorthand{ grow: 1.0, shrink: 1.0, basis: b };
            }
            return err.InvalidFlexShorthand;
        }
        if (n == 2) {
            const g = parseNonNegNumber(toks[0]) ?? return err.InvalidFlexShorthand;
            if (sh := parseNonNegNumber(toks[1])) {
                return FlexShorthand{ grow: g, shrink: sh, basis: FlexBasis.percent(0.0) };
            }
            if (b := FlexBasis.parse(toks[1])) {
                return FlexShorthand{ grow: g, shrink: 1.0, basis: b };
            }
            return err.InvalidFlexShorthand;
        }
        // n == 3: <grow> <shrink> <basis>
        const g = parseNonNegNumber(toks[0]) ?? return err.InvalidFlexShorthand;
        const sh = parseNonNegNumber(toks[1]) ?? return err.InvalidFlexShorthand;
        const b = FlexBasis.parse(toks[2]) ?? return err.InvalidFlexShorthand;
        return FlexShorthand{ grow: g, shrink: sh, basis: b };
    }

    fn toCss(buf: []u8): usize ! CssLayoutError {
        if (_.basis.kind == FlexBasisKind.auto) {
            if (_.grow == 0.0 && _.shrink == 0.0) {
                const s = std.fmt.bufPrint(buf, "none", .{}) catch return err.BufferTooSmall;
                return s.len;
            }
            if (_.grow == 1.0 && _.shrink == 1.0) {
                const s = std.fmt.bufPrint(buf, "auto", .{}) catch return err.BufferTooSmall;
                return s.len;
            }
            if (_.grow == 0.0 && _.shrink == 1.0) {
                const s = std.fmt.bufPrint(buf, "initial", .{}) catch return err.BufferTooSmall;
                return s.len;
            }
        }
        let bbuf: [32]u8 = std.mem.zeroes([32]u8);
        const blen = _.basis.toCss(bbuf[0..])?;
        const s = std.fmt.bufPrint(buf, "{d} {d} {s}", .{ _.grow, _.shrink, bbuf[0..blen] }) catch return err.BufferTooSmall;
        return s.len;
    }
}

/// CSS `justify-content`
pub enum JustifyContent: u8 {
    normal = 0,
    flex_start = 1,
    flex_end = 2,
    center = 3,
    space_between = 4,
    space_around = 5,
    space_evenly = 6,
    start = 7,
    end = 8,
    left = 9,
    right = 10,
    stretch = 11,

    static fn default(): JustifyContent { return .normal; }

    static fn parse(input: string): JustifyContent? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "flex-start")) { return .flex_start; }
        if (std.ascii.eqlIgnoreCase(s, "flex-end")) { return .flex_end; }
        if (std.ascii.eqlIgnoreCase(s, "center")) { return .center; }
        if (std.ascii.eqlIgnoreCase(s, "space-between")) { return .space_between; }
        if (std.ascii.eqlIgnoreCase(s, "space-around")) { return .space_around; }
        if (std.ascii.eqlIgnoreCase(s, "space-evenly")) { return .space_evenly; }
        if (std.ascii.eqlIgnoreCase(s, "start")) { return .start; }
        if (std.ascii.eqlIgnoreCase(s, "end")) { return .end; }
        if (std.ascii.eqlIgnoreCase(s, "left")) { return .left; }
        if (std.ascii.eqlIgnoreCase(s, "right")) { return .right; }
        if (std.ascii.eqlIgnoreCase(s, "stretch")) { return .stretch; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .normal => "normal",
            .flex_start => "flex-start",
            .flex_end => "flex-end",
            .center => "center",
            .space_between => "space-between",
            .space_around => "space-around",
            .space_evenly => "space-evenly",
            .start => "start",
            .end => "end",
            .left => "left",
            .right => "right",
            .stretch => "stretch",
        };
    }
}

/// CSS `align-items`
pub enum AlignItems: u8 {
    normal = 0,
    stretch = 1,
    flex_start = 2,
    flex_end = 3,
    center = 4,
    baseline = 5,
    start = 6,
    end = 7,
    self_start = 8,
    self_end = 9,

    static fn default(): AlignItems { return .normal; }

    static fn parse(input: string): AlignItems? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "stretch")) { return .stretch; }
        if (std.ascii.eqlIgnoreCase(s, "flex-start")) { return .flex_start; }
        if (std.ascii.eqlIgnoreCase(s, "flex-end")) { return .flex_end; }
        if (std.ascii.eqlIgnoreCase(s, "center")) { return .center; }
        if (std.ascii.eqlIgnoreCase(s, "baseline")) { return .baseline; }
        if (std.ascii.eqlIgnoreCase(s, "start")) { return .start; }
        if (std.ascii.eqlIgnoreCase(s, "end")) { return .end; }
        if (std.ascii.eqlIgnoreCase(s, "self-start")) { return .self_start; }
        if (std.ascii.eqlIgnoreCase(s, "self-end")) { return .self_end; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .normal => "normal",
            .stretch => "stretch",
            .flex_start => "flex-start",
            .flex_end => "flex-end",
            .center => "center",
            .baseline => "baseline",
            .start => "start",
            .end => "end",
            .self_start => "self-start",
            .self_end => "self-end",
        };
    }
}

/// CSS `align-self`
pub enum AlignSelf: u8 {
    auto = 0,
    normal = 1,
    stretch = 2,
    flex_start = 3,
    flex_end = 4,
    center = 5,
    baseline = 6,
    start = 7,
    end = 8,
    self_start = 9,
    self_end = 10,

    static fn default(): AlignSelf { return .auto; }

    static fn parse(input: string): AlignSelf? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "auto")) { return .auto; }
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "stretch")) { return .stretch; }
        if (std.ascii.eqlIgnoreCase(s, "flex-start")) { return .flex_start; }
        if (std.ascii.eqlIgnoreCase(s, "flex-end")) { return .flex_end; }
        if (std.ascii.eqlIgnoreCase(s, "center")) { return .center; }
        if (std.ascii.eqlIgnoreCase(s, "baseline")) { return .baseline; }
        if (std.ascii.eqlIgnoreCase(s, "start")) { return .start; }
        if (std.ascii.eqlIgnoreCase(s, "end")) { return .end; }
        if (std.ascii.eqlIgnoreCase(s, "self-start")) { return .self_start; }
        if (std.ascii.eqlIgnoreCase(s, "self-end")) { return .self_end; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .auto => "auto",
            .normal => "normal",
            .stretch => "stretch",
            .flex_start => "flex-start",
            .flex_end => "flex-end",
            .center => "center",
            .baseline => "baseline",
            .start => "start",
            .end => "end",
            .self_start => "self-start",
            .self_end => "self-end",
        };
    }
}

/// CSS `align-content`
pub enum AlignContent: u8 {
    normal = 0,
    flex_start = 1,
    flex_end = 2,
    center = 3,
    space_between = 4,
    space_around = 5,
    space_evenly = 6,
    stretch = 7,
    start = 8,
    end = 9,
    baseline = 10,

    static fn default(): AlignContent { return .normal; }

    static fn parse(input: string): AlignContent? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "flex-start")) { return .flex_start; }
        if (std.ascii.eqlIgnoreCase(s, "flex-end")) { return .flex_end; }
        if (std.ascii.eqlIgnoreCase(s, "center")) { return .center; }
        if (std.ascii.eqlIgnoreCase(s, "space-between")) { return .space_between; }
        if (std.ascii.eqlIgnoreCase(s, "space-around")) { return .space_around; }
        if (std.ascii.eqlIgnoreCase(s, "space-evenly")) { return .space_evenly; }
        if (std.ascii.eqlIgnoreCase(s, "stretch")) { return .stretch; }
        if (std.ascii.eqlIgnoreCase(s, "start")) { return .start; }
        if (std.ascii.eqlIgnoreCase(s, "end")) { return .end; }
        if (std.ascii.eqlIgnoreCase(s, "baseline")) { return .baseline; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .normal => "normal",
            .flex_start => "flex-start",
            .flex_end => "flex-end",
            .center => "center",
            .space_between => "space-between",
            .space_around => "space-around",
            .space_evenly => "space-evenly",
            .stretch => "stretch",
            .start => "start",
            .end => "end",
            .baseline => "baseline",
        };
    }
}

/// CSS `grid-auto-flow`（`row` / `column` / `dense` / `row dense` / `column dense`）
pub enum GridAutoFlow: u8 {
    row = 0,
    column = 1,
    row_dense = 2,
    column_dense = 3,

    static fn default(): GridAutoFlow {
        return .row;
    }

    static fn parse(input: string): GridAutoFlow ! CssLayoutError {
        let toks: [4]string = ["", "", "", ""];
        const n = splitWhitespaceTokens(input, &toks);
        if (n == 0 || n > 2) {
            return err.InvalidGridAutoFlow;
        }
        let has_row = false;
        let has_col = false;
        let has_dense = false;
        let i: usize = 0;
        while (i < n) {
            if (std.ascii.eqlIgnoreCase(toks[i], "row")) {
                if (has_row || has_col) { return err.InvalidGridAutoFlow; }
                has_row = true;
            } else if (std.ascii.eqlIgnoreCase(toks[i], "column")) {
                if (has_row || has_col) { return err.InvalidGridAutoFlow; }
                has_col = true;
            } else if (std.ascii.eqlIgnoreCase(toks[i], "dense")) {
                if (has_dense) { return err.InvalidGridAutoFlow; }
                has_dense = true;
            } else {
                return err.InvalidGridAutoFlow;
            }
            i += 1;
        }
        if (has_col) {
            return if (has_dense) GridAutoFlow.column_dense else GridAutoFlow.column;
        }
        return if (has_dense) GridAutoFlow.row_dense else GridAutoFlow.row;
    }

    fn toCss(): string {
        return switch (_) {
            .row => "row",
            .column => "column",
            .row_dense => "row dense",
            .column_dense => "column dense",
        };
    }

    fn isDense(): bool {
        return _ == .row_dense || _ == .column_dense;
    }

    fn isColumn(): bool {
        return _ == .column || _ == .column_dense;
    }

    fn isRow(): bool {
        return _ == .row || _ == .row_dense;
    }
}

/// CSS Grid `<track-breadth>` 类型
pub enum TrackBreadthKind: u8 {
    auto = 0,
    min_content = 1,
    max_content = 2,
    fr = 3,
    px = 4,
    percent = 5,
}

/// CSS Grid `<track-breadth>` 值（`auto` / `min-content` / `max-content` / `<fr>` / `<px>` / `<%>`）
pub struct TrackBreadth {
    kind: TrackBreadthKind = TrackBreadthKind.auto,
    value: f64 = 0.0,

    static fn parse(input: string): TrackBreadth ! CssLayoutError {
        const s = trimAscii(input);
        if (s.len == 0) {
            return err.InvalidTrackBreadth;
        }
        if (std.ascii.eqlIgnoreCase(s, "auto")) {
            return TrackBreadth{ kind: TrackBreadthKind.auto };
        }
        if (std.ascii.eqlIgnoreCase(s, "min-content")) {
            return TrackBreadth{ kind: TrackBreadthKind.min_content };
        }
        if (std.ascii.eqlIgnoreCase(s, "max-content")) {
            return TrackBreadth{ kind: TrackBreadthKind.max_content };
        }
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "fr")) {
            const v = parseNonNegNumber(s[0 .. s.len - 2]) ?? return err.InvalidTrackBreadth;
            return TrackBreadth{ kind: TrackBreadthKind.fr, value: v };
        }
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "px")) {
            const v = parseNonNegNumber(s[0 .. s.len - 2]) ?? return err.InvalidTrackBreadth;
            return TrackBreadth{ kind: TrackBreadthKind.px, value: v };
        }
        if (s.len > 1 && s[s.len - 1] == '%') {
            const v = parseNonNegNumber(s[0 .. s.len - 1]) ?? return err.InvalidTrackBreadth;
            return TrackBreadth{ kind: TrackBreadthKind.percent, value: v };
        }
        return err.InvalidTrackBreadth;
    }

    fn isFlexible(): bool {
        return _.kind == TrackBreadthKind.fr;
    }

    fn isIntrinsic(): bool {
        return _.kind == TrackBreadthKind.auto
            || _.kind == TrackBreadthKind.min_content
            || _.kind == TrackBreadthKind.max_content;
    }

    fn toCss(buf: []u8): usize ! CssLayoutError {
        const res = switch (_.kind) {
            .auto => std.fmt.bufPrint(buf, "auto", .{}),
            .min_content => std.fmt.bufPrint(buf, "min-content", .{}),
            .max_content => std.fmt.bufPrint(buf, "max-content", .{}),
            .fr => std.fmt.bufPrint(buf, "{d}fr", .{_.value}),
            .px => std.fmt.bufPrint(buf, "{d}px", .{_.value}),
            .percent => std.fmt.bufPrint(buf, "{d}%", .{_.value}),
        } catch return err.BufferTooSmall;
        return res.len;
    }
}
