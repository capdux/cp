import { test, expect_eq } from "cap:test";
import {
    CssLayoutError,
    FlexDirection,
    FlexWrap,
    FlexFlow,
    FlexBasisKind,
    FlexShorthand,
    JustifyContent,
    AlignItems,
    AlignSelf,
    AlignContent,
    GridAutoFlow,
    TrackBreadthKind,
    TrackBreadth,
} from "cap:css_flex_grid";

test("1. FlexDirection (4 variants), FlexWrap (3 variants) & FlexFlow shorthand parse/normalize", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    expect_eq(FlexDirection.parse("ROW-REVERSE").? == FlexDirection.row_reverse, true);
    expect_eq(FlexDirection.row_reverse.isHorizontal(), true);
    expect_eq(FlexDirection.row_reverse.isReversed(), true);
    expect_eq(FlexDirection.column.isVertical(), true);
    expect_eq(FlexWrap.wrap_reverse.isMultiLine(), true);
    expect_eq(FlexWrap.nowrap.isMultiLine(), false);

    // Order-independent FlexFlow parsing
    const f1 = FlexFlow.parse("wrap column-reverse") catch unreachable;
    expect_eq(f1.direction == FlexDirection.column_reverse, true);
    expect_eq(f1.wrap == FlexWrap.wrap, true);
    const n1 = f1.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "column-reverse wrap"), true);

    // Single token defaults omitted in canonical output
    const f2 = FlexFlow.parse("wrap") catch unreachable;
    expect_eq(f2.direction == FlexDirection.row, true);
    const n2 = f2.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "wrap"), true);
});

test("2. FlexShorthand (none/auto/initial/<grow> <shrink>? <basis>?) parse & defaults", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const f_none = FlexShorthand.parse("none") catch unreachable;
    expect_eq(f_none.grow, 0.0);
    expect_eq(f_none.shrink, 0.0);
    expect_eq(f_none.basis.kind == FlexBasisKind.auto, true);
    const n_none = f_none.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_none], "none"), true);

    const f_auto = FlexShorthand.parse("auto") catch unreachable;
    expect_eq(f_auto.grow, 1.0);
    expect_eq(f_auto.shrink, 1.0);
    const n_auto = f_auto.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_auto], "auto"), true);

    // Single number `flex: 2` => 2 1 0%
    const f_one = FlexShorthand.parse("2") catch unreachable;
    expect_eq(f_one.grow, 2.0);
    expect_eq(f_one.shrink, 1.0);
    expect_eq(f_one.basis.kind == FlexBasisKind.percent, true);
    expect_eq(f_one.basis.value, 0.0);

    // Three tokens `flex: 1 0 240px`
    const f_three = FlexShorthand.parse("1 0 240px") catch unreachable;
    expect_eq(f_three.grow, 1.0);
    expect_eq(f_three.shrink, 0.0);
    expect_eq(f_three.basis.kind == FlexBasisKind.px, true);
    expect_eq(f_three.basis.value, 240.0);
    const n3 = f_three.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n3], "1 0 240px"), true);
});

test("3. JustifyContent, AlignItems, AlignSelf & AlignContent parse & CSS serialization", () => {
    expect_eq(JustifyContent.parse("SPACE-BETWEEN").? == JustifyContent.space_between, true);
    expect_eq(std.mem.eql(u8, JustifyContent.space_evenly.toCss(), "space-evenly"), true);

    expect_eq(AlignItems.parse("center").? == AlignItems.center, true);
    expect_eq(std.mem.eql(u8, AlignItems.flex_start.toCss(), "flex-start"), true);

    expect_eq(AlignSelf.default() == AlignSelf.auto, true);
    expect_eq(AlignSelf.parse("self-end").? == AlignSelf.self_end, true);
    expect_eq(std.mem.eql(u8, AlignSelf.baseline.toCss(), "baseline"), true);

    expect_eq(AlignContent.parse("stretch").? == AlignContent.stretch, true);
    expect_eq(std.mem.eql(u8, AlignContent.space_around.toCss(), "space-around"), true);
});

test("4. GridAutoFlow & TrackBreadth (auto/min-content/max-content/fr/px/%) parse & serialize", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const g1 = GridAutoFlow.parse("dense") catch unreachable;
    expect_eq(g1 == GridAutoFlow.row_dense, true);
    expect_eq(g1.isDense(), true);
    expect_eq(g1.isRow(), true);

    const g2 = GridAutoFlow.parse("dense column") catch unreachable;
    expect_eq(g2 == GridAutoFlow.column_dense, true);
    expect_eq(g2.isColumn(), true);
    expect_eq(std.mem.eql(u8, g2.toCss(), "column dense"), true);

    const tb_fr = TrackBreadth.parse("2fr") catch unreachable;
    expect_eq(tb_fr.kind == TrackBreadthKind.fr, true);
    expect_eq(tb_fr.isFlexible(), true);
    expect_eq(tb_fr.isIntrinsic(), false);
    const n_fr = tb_fr.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_fr], "2fr"), true);

    const tb_min = TrackBreadth.parse("min-content") catch unreachable;
    expect_eq(tb_min.isIntrinsic(), true);
    const tb_pct = TrackBreadth.parse("25%") catch unreachable;
    expect_eq(tb_pct.kind == TrackBreadthKind.percent, true);
    expect_eq(tb_pct.value, 25.0);
});

test("5. Malformed & duplicate token boundary guards", () => {
    std.testing.expectError(CssLayoutError.InvalidFlexFlow, FlexFlow.parse("")) catch unreachable;
    std.testing.expectError(CssLayoutError.InvalidFlexFlow, FlexFlow.parse("row column")) catch unreachable;
    std.testing.expectError(CssLayoutError.InvalidFlexFlow, FlexFlow.parse("wrap nowrap")) catch unreachable;

    std.testing.expectError(CssLayoutError.InvalidFlexShorthand, FlexShorthand.parse("")) catch unreachable;
    std.testing.expectError(CssLayoutError.InvalidFlexShorthand, FlexShorthand.parse("-1 1 auto")) catch unreachable;
    std.testing.expectError(CssLayoutError.InvalidFlexShorthand, FlexShorthand.parse("1 1 1 1")) catch unreachable;

    std.testing.expectError(CssLayoutError.InvalidGridAutoFlow, GridAutoFlow.parse("row column")) catch unreachable;
    std.testing.expectError(CssLayoutError.InvalidGridAutoFlow, GridAutoFlow.parse("dense dense")) catch unreachable;
    std.testing.expectError(CssLayoutError.InvalidTrackBreadth, TrackBreadth.parse("-2fr")) catch unreachable;
    std.testing.expectError(CssLayoutError.InvalidTrackBreadth, TrackBreadth.parse("10em")) catch unreachable;
});
