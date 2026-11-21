import { test, expect_eq } from "cap:test";
import {
    CssContainerError,
    ContainerType,
    ContainerName,
    ContainerSizeFeatureId,
    Orientation,
    ContainerSizeFeature,
    ContainerQuery,
} from "cap:css_container";

test("1. ContainerType (normal/size/inline-size) parse, serialize & containment predicates", () => {
    const t_norm = ContainerType.parse("normal").?;
    expect_eq(t_norm == ContainerType.default(), true);
    expect_eq(t_norm.supportsSizeQueries(), false);
    expect_eq(t_norm.supportsBlockQueries(), false);

    const t_inline = ContainerType.parse("INLINE-SIZE").?;
    expect_eq(t_inline.supportsSizeQueries(), true);
    expect_eq(t_inline.supportsBlockQueries(), false);
    expect_eq(std.mem.eql(u8, t_inline.toCss(), "inline-size"), true);

    const t_size = ContainerType.parse("size").?;
    expect_eq(t_size.supportsSizeQueries(), true);
    expect_eq(t_size.supportsBlockQueries(), true);
    expect_eq(ContainerType.parse("block-size") == null, true);
});

test("2. ContainerName validation forbids none/and/not/or keywords", () => {
    const valid = ContainerName.parse("sidebar-main") catch unreachable;
    expect_eq(std.mem.eql(u8, valid.toCss(), "sidebar-main"), true);

    std.testing.expectError(CssContainerError.InvalidContainerName, ContainerName.parse("none")) catch unreachable;
    std.testing.expectError(CssContainerError.InvalidContainerName, ContainerName.parse("AND")) catch unreachable;
    std.testing.expectError(CssContainerError.InvalidContainerName, ContainerName.parse("not")) catch unreachable;
    std.testing.expectError(CssContainerError.InvalidContainerName, ContainerName.parse("Or")) catch unreachable;
    std.testing.expectError(CssContainerError.InvalidContainerName, ContainerName.parse("123bad")) catch unreachable;
    std.testing.expectError(CssContainerError.InvalidContainerName, ContainerName.parse("")) catch unreachable;
});

test("3. ContainerSizeFeature parse & min-/max-/range evaluation (width/height/inline-size/aspect-ratio/orientation)", () => {
    const f_min_w = ContainerSizeFeature.parse("(min-width: 400px)") catch unreachable;
    expect_eq(f_min_w.feature == ContainerSizeFeatureId.width, true);
    expect_eq(f_min_w.evaluate(400.0, 300.0), true);
    expect_eq(f_min_w.evaluate(399.0, 300.0), false);

    // Modern range syntax + rem unit (30rem = 480px)
    const f_inline = ContainerSizeFeature.parse("(inline-size < 30rem)") catch unreachable;
    expect_eq(f_inline.evaluate(400.0, 200.0), true);
    expect_eq(f_inline.evaluate(480.0, 200.0), false);

    // aspect-ratio
    const f_ar = ContainerSizeFeature.parse("(min-aspect-ratio: 16/9)") catch unreachable;
    expect_eq(f_ar.evaluate(1920.0, 1080.0), true);
    expect_eq(f_ar.evaluate(800.0, 600.0), false);
    expect_eq(f_ar.evaluate(800.0, 0.0), false);

    // orientation
    const f_ori = ContainerSizeFeature.parse("(orientation: landscape)") catch unreachable;
    expect_eq(f_ori.orientation == Orientation.landscape, true);
    expect_eq(f_ori.evaluate(800.0, 600.0), true);
    expect_eq(f_ori.evaluate(600.0, 800.0), false);
});

test("4. ContainerQuery and/or/not logical evaluation & canonical toCss serialization", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    // Named container + `and`
    const q1 = ContainerQuery.parse("sidebar (min-width: 300px) and (max-width: 600px)") catch unreachable;
    expect_eq(std.mem.eql(u8, q1.name.?.toCss(), "sidebar"), true);
    expect_eq(q1.evaluate(450.0, 200.0), true);
    expect_eq(q1.evaluate(250.0, 200.0), false);
    expect_eq(q1.evaluate(700.0, 200.0), false);

    const n1 = q1.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "@container sidebar (min-width: 300px) and (max-width: 600px)"), true);

    // `or` combinator
    const q2 = ContainerQuery.parse("(width >= 800px) or (orientation: portrait)") catch unreachable;
    expect_eq(q2.evaluate(900.0, 400.0), true);
    expect_eq(q2.evaluate(300.0, 600.0), true);
    expect_eq(q2.evaluate(500.0, 400.0), false);

    // `not` negation
    const q3 = ContainerQuery.parse("not (orientation: portrait)") catch unreachable;
    expect_eq(q3.evaluate(800.0, 400.0), true);
    expect_eq(q3.evaluate(400.0, 800.0), false);
    const n3 = q3.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n3], "@container not (orientation: portrait)"), true);
});

test("5. Malformed syntax, mixed and/or & invalid feature value guards", () => {
    std.testing.expectError(CssContainerError.InvalidQuerySyntax, ContainerQuery.parse("")) catch unreachable;
    std.testing.expectError(CssContainerError.InvalidQuerySyntax, ContainerQuery.parse("(min-width: 300px) and")) catch unreachable;
    // Mixing `and` and `or` without parens rejected
    std.testing.expectError(
        CssContainerError.InvalidQuerySyntax,
        ContainerQuery.parse("(min-width: 100px) and (max-width: 500px) or (orientation: portrait)"),
    ) catch unreachable;
    // Forbidden container name in query
    std.testing.expectError(
        CssContainerError.InvalidContainerName,
        ContainerQuery.parse("none (min-width: 300px)"),
    ) catch unreachable;
    // Unknown feature & bad values
    std.testing.expectError(CssContainerError.InvalidFeatureName, ContainerSizeFeature.parse("(color: 8)")) catch unreachable;
    std.testing.expectError(CssContainerError.InvalidFeatureValue, ContainerSizeFeature.parse("(min-width: -10px)")) catch unreachable;
    std.testing.expectError(CssContainerError.InvalidFeatureValue, ContainerSizeFeature.parse("(aspect-ratio: 16/0)")) catch unreachable;
    std.testing.expectError(CssContainerError.InvalidFeatureValue, ContainerSizeFeature.parse("(orientation: diagonal)")) catch unreachable;
});
