import { test, expect_eq } from "cap:test";
import {
    CssAtRuleError,
    isValidKeyframesIdent,
    TimelineRangeName,
    KeyframeSelectorKind,
    KeyframeSelector,
    LayerName,
    CounterSystem,
    PropertySyntaxKind,
    isValidCustomPropertyName,
    PropertyDescriptor,
} from "cap:css_at_rules";

test("1. KeyframeSelector (from/to/0%..100%/timeline-range) parse, normalize & minify serialization", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    expect_eq(isValidKeyframesIdent("fade-in"), true);
    expect_eq(isValidKeyframesIdent("none"), false);
    expect_eq(isValidKeyframesIdent("revert-layer"), false);

    const k_from = KeyframeSelector.parse("from") catch unreachable;
    expect_eq(k_from.kind == KeyframeSelectorKind.from_kw, true);
    expect_eq(k_from.normalizedPercent(), 0.0);
    const n1 = k_from.toCss(buf[0..], false) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "from"), true);
    const n1_min = k_from.toCss(buf[0..], true) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1_min], "0%"), true);

    const k_100 = KeyframeSelector.parse("100%") catch unreachable;
    expect_eq(k_100.normalizedPercent(), 100.0);
    const n2_min = k_100.toCss(buf[0..], true) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2_min], "to"), true);

    const k_mid = KeyframeSelector.parse("50%") catch unreachable;
    expect_eq(k_mid.normalizedPercent(), 50.0);

    // Scroll-driven timeline range selector
    const k_tr = KeyframeSelector.parse("entry-crossing 25%") catch unreachable;
    expect_eq(k_tr.kind == KeyframeSelectorKind.timeline_range, true);
    expect_eq(k_tr.range_name.? == TimelineRangeName.entry_crossing, true);
    expect_eq(k_tr.normalizedPercent(), 25.0);
    const n_tr = k_tr.toCss(buf[0..], false) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_tr], "entry-crossing 25%"), true);
});

test("2. LayerName dotted multi-segment parse, prefix matching, equality, serialize & anonymous layer", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const anon = LayerName.anonymous();
    expect_eq(anon.isAnonymous(), true);
    expect_eq(anon.depth(), usize(0));

    const base = LayerName.parse("framework") catch unreachable;
    const sub = LayerName.parse("framework.theme.dark") catch unreachable;
    const other = LayerName.parse("utilities.spacing") catch unreachable;

    expect_eq(base.isAnonymous(), false);
    expect_eq(sub.depth(), usize(3));
    expect_eq(base.isPrefixOf(sub), true);
    expect_eq(sub.isPrefixOf(base), false);
    expect_eq(base.isPrefixOf(other), false);
    expect_eq(anon.isPrefixOf(sub), false);

    const sub_copy = LayerName.parse("framework.theme.dark") catch unreachable;
    expect_eq(sub.eql(sub_copy), true);
    expect_eq(sub.eql(other), false);

    const n = sub.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n], "framework.theme.dark"), true);
});

test("3. CounterSystem all 7 systems parse, CSS serialize & symbol constraints", () => {
    expect_eq(CounterSystem.default() == CounterSystem.symbolic, true);
    expect_eq(CounterSystem.parse("CYCLIC").? == CounterSystem.cyclic, true);
    expect_eq(CounterSystem.parse("numeric").? == CounterSystem.numeric, true);
    expect_eq(CounterSystem.parse("alphabetic").? == CounterSystem.alphabetic, true);
    expect_eq(CounterSystem.parse("symbolic").? == CounterSystem.symbolic, true);
    expect_eq(CounterSystem.parse("additive").? == CounterSystem.additive, true);
    expect_eq(CounterSystem.parse("fixed").? == CounterSystem.fixed, true);
    expect_eq(CounterSystem.parse("extends").? == CounterSystem.extends_style, true);

    expect_eq(CounterSystem.numeric.requiresSymbols(), true);
    expect_eq(CounterSystem.numeric.requiresMinTwoSymbols(), true);
    expect_eq(CounterSystem.alphabetic.requiresMinTwoSymbols(), true);
    expect_eq(CounterSystem.cyclic.requiresMinTwoSymbols(), false);
    expect_eq(CounterSystem.additive.requiresSymbols(), false);
    expect_eq(CounterSystem.extends_style.requiresSymbols(), false);
    expect_eq(std.mem.eql(u8, CounterSystem.additive.toCss(), "additive"), true);
});

test("4. PropertySyntaxKind & PropertyDescriptor (@property) validation and inherits/initial-value rules", () => {
    expect_eq(isValidCustomPropertyName("--primary-color"), true);
    expect_eq(isValidCustomPropertyName("--"), false);
    expect_eq(isValidCustomPropertyName("-single"), false);

    // Non-universal syntax requires initial-value
    const p_color = PropertyDescriptor.create("--brand", "\"<color>\"", true, "#ff0088") catch unreachable;
    expect_eq(p_color.syntax == PropertySyntaxKind.color, true);
    expect_eq(p_color.inherits, true);
    expect_eq(std.mem.eql(u8, p_color.syntax.toCss(), "<color>"), true);

    // Universal `*` syntax allows null initial-value
    const p_univ = PropertyDescriptor.create("--any-val", "'*'", false, null) catch unreachable;
    expect_eq(p_univ.syntax.isUniversal(), true);
    expect_eq(p_univ.inherits, false);
    expect_eq(p_univ.initial_value == null, true);

    // Missing initial-value for non-universal syntax rejected
    std.testing.expectError(
        CssAtRuleError.MissingInitialValue,
        PropertyDescriptor.create("--size", "<length>", true, null),
    ) catch unreachable;
});

test("5. Malformed & out-of-range input guards across all @-rule models", () => {
    // KeyframeSelector out of range & malformed
    std.testing.expectError(CssAtRuleError.InvalidKeyframeSelector, KeyframeSelector.parse("")) catch unreachable;
    std.testing.expectError(CssAtRuleError.InvalidKeyframeSelector, KeyframeSelector.parse("-5%")) catch unreachable;
    std.testing.expectError(CssAtRuleError.InvalidKeyframeSelector, KeyframeSelector.parse("101%")) catch unreachable;
    std.testing.expectError(CssAtRuleError.InvalidKeyframeSelector, KeyframeSelector.parse("bogus 50%")) catch unreachable;

    // LayerName empty segment, leading/trailing dot, >8 segments
    std.testing.expectError(CssAtRuleError.InvalidLayerName, LayerName.parse("")) catch unreachable;
    std.testing.expectError(CssAtRuleError.InvalidLayerName, LayerName.parse(".base")) catch unreachable;
    std.testing.expectError(CssAtRuleError.InvalidLayerName, LayerName.parse("base.")) catch unreachable;
    std.testing.expectError(CssAtRuleError.InvalidLayerName, LayerName.parse("a..b")) catch unreachable;
    std.testing.expectError(CssAtRuleError.TooManyLayerSegments, LayerName.parse("a.b.c.d.e.f.g.h.i")) catch unreachable;

    // CounterSystem & PropertyDescriptor invalid inputs
    expect_eq(CounterSystem.parse("roman") == null, true);
    std.testing.expectError(
        CssAtRuleError.InvalidPropertyName,
        PropertyDescriptor.create("not-dashed", "*", true, null),
    ) catch unreachable;
    std.testing.expectError(
        CssAtRuleError.InvalidPropertySyntax,
        PropertyDescriptor.create("--x", "<bogus>", true, "1"),
    ) catch unreachable;
});
