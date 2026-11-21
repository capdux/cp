import { test, expect_eq } from "cap:test";
import {
    CssEasingError,
    StepPosition,
    EasingKind,
    EasingFunction,
} from "cap:css_easing";

fn approxClose(a: f64, b: f64, eps: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff <= eps;
}

test("1. Easing keywords (linear/ease/ease-in/ease-out/ease-in-out/step-start/step-end) parse & serialize", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    expect_eq(EasingFunction.isIdent("linear"), true);
    expect_eq(EasingFunction.isIdent("EASE-IN-OUT"), true);
    expect_eq(EasingFunction.isIdent("step-start"), true);
    expect_eq(EasingFunction.isIdent("bounce"), false);

    const k_lin = EasingFunction.parse("  LINEAR  ") catch unreachable;
    expect_eq(k_lin.kind == EasingKind.linear, true);
    const n1 = k_lin.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "linear"), true);

    const k_ss = EasingFunction.parse("step-start") catch unreachable;
    expect_eq(k_ss.kind == EasingKind.steps, true);
    expect_eq(k_ss.steps.count, u32(1));
    expect_eq(k_ss.steps.position == StepPosition.start, true);
    const n2 = k_ss.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "step-start"), true);

    const k_se = EasingFunction.parse("step-end") catch unreachable;
    const n3 = k_se.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n3], "step-end"), true);
});

test("2. cubic-bezier() parsing, keyword normalization & custom serialization", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    // Equivalent to `ease`
    const cb_ease = EasingFunction.parse("cubic-bezier(0.25, 0.1, 0.25, 1)") catch unreachable;
    expect_eq(cb_ease.isEase(), true);
    const n_ease = cb_ease.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n_ease], "ease"), true);

    // Equivalent to `ease-in`, `ease-out`, `ease-in-out`, `linear`
    const cb_in = EasingFunction.parse("cubic-bezier(0.42, 0, 1, 1)") catch unreachable;
    expect_eq(cb_in.normalize().kind == EasingKind.ease_in, true);

    const cb_out = EasingFunction.parse("cubic-bezier(0, 0, 0.58, 1)") catch unreachable;
    expect_eq(cb_out.normalize().kind == EasingKind.ease_out, true);

    const cb_inout = EasingFunction.parse("cubic-bezier(0.42, 0, 0.58, 1)") catch unreachable;
    expect_eq(cb_inout.normalize().kind == EasingKind.ease_in_out, true);

    const cb_lin = EasingFunction.parse("cubic-bezier(0, 0, 1, 1)") catch unreachable;
    expect_eq(cb_lin.normalize().kind == EasingKind.linear, true);

    // Custom curve with overshoot y > 1.0 stays cubic-bezier
    const cb_custom = EasingFunction.parse("cubic-bezier(0.34, 1.56, 0.64, 1)") catch unreachable;
    expect_eq(cb_custom.normalize().kind == EasingKind.cubic_bezier, true);
    const n_cust = cb_custom.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.startsWith(u8, buf[0..n_cust], "cubic-bezier("), true);
});

test("3. steps(n, pos) parsing across all jump-* / start / end variants & serialization", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    // Default position is `end`
    const s_def = EasingFunction.parse("steps(4)") catch unreachable;
    expect_eq(s_def.steps.count, u32(4));
    expect_eq(s_def.steps.position == StepPosition.end, true);
    const n1 = s_def.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1], "steps(4,end)"), true);

    // jump-start aliases to start, jump-end aliases to end
    const s_js = EasingFunction.parse("steps(5, jump-start)") catch unreachable;
    expect_eq(s_js.steps.position == StepPosition.start, true);
    const n2 = s_js.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "steps(5,start)"), true);

    const s_je = EasingFunction.parse("steps(1, jump-end)") catch unreachable;
    const n3 = s_je.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n3], "step-end"), true);

    const s_jn = EasingFunction.parse("steps(4, jump-none)") catch unreachable;
    expect_eq(s_jn.steps.position == StepPosition.jump_none, true);
    const n4 = s_jn.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n4], "steps(4,jump-none)"), true);

    const s_jb = EasingFunction.parse("steps(3, jump-both)") catch unreachable;
    expect_eq(s_jb.steps.position == StepPosition.jump_both, true);
    const n5 = s_jb.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n5], "steps(3,jump-both)"), true);
});

test("4. sample(t) numerical evaluation for linear, cubic-bezier & steps curves", () => {
    // linear
    const lin = EasingFunction.linear();
    expect_eq(approxClose(lin.sample(0.0), 0.0, 0.000000001), true);
    expect_eq(approxClose(lin.sample(0.5), 0.5, 0.000000001), true);
    expect_eq(approxClose(lin.sample(1.0), 1.0, 0.000000001), true);
    expect_eq(approxClose(lin.sample(-0.5), 0.0, 0.000000001), true);
    expect_eq(approxClose(lin.sample(1.5), 1.0, 0.000000001), true);

    // symmetric ease-in-out at t=0.5 is exactly 0.5
    const eio = EasingFunction.easeInOut();
    expect_eq(approxClose(eio.sample(0.0), 0.0, 0.000000001), true);
    expect_eq(approxClose(eio.sample(0.5), 0.5, 0.000001), true);
    expect_eq(approxClose(eio.sample(1.0), 1.0, 0.000000001), true);

    // ease at t=0.5 is ~0.8024
    const ease_fn = EasingFunction.ease();
    expect_eq(approxClose(ease_fn.sample(0.5), 0.8024, 0.001), true);

    // ease-in at t=0.5 is slower (< 0.5), ease-out at t=0.5 is faster (> 0.5)
    expect_eq(EasingFunction.easeIn().sample(0.5) < 0.5, true);
    expect_eq(EasingFunction.easeOut().sample(0.5) > 0.5, true);

    // steps(4, end) vs steps(4, start) vs steps(4, jump-none) vs steps(4, jump-both)
    const st_end = EasingFunction.makeSteps(4, StepPosition.end) catch unreachable;
    expect_eq(approxClose(st_end.sample(0.0), 0.0, 0.000000001), true);
    expect_eq(approxClose(st_end.sample(0.5), 0.5, 0.000000001), true);
    expect_eq(approxClose(st_end.sample(1.0), 1.0, 0.000000001), true);

    const st_start = EasingFunction.makeSteps(4, StepPosition.start) catch unreachable;
    expect_eq(approxClose(st_start.sample(0.0), 0.25, 0.000000001), true);
    expect_eq(approxClose(st_start.sample(0.5), 0.75, 0.000000001), true);
    expect_eq(approxClose(st_start.sample(1.0), 1.0, 0.000000001), true);

    const st_none = EasingFunction.makeSteps(4, StepPosition.jump_none) catch unreachable;
    expect_eq(approxClose(st_none.sample(0.0), 0.0, 0.000000001), true);
    expect_eq(approxClose(st_none.sample(1.0), 1.0, 0.000000001), true);

    const st_both = EasingFunction.makeSteps(4, StepPosition.jump_both) catch unreachable;
    expect_eq(approxClose(st_both.sample(0.0), 0.2, 0.000000001), true);
    expect_eq(approxClose(st_both.sample(1.0), 1.0, 0.000000001), true);
});

test("5. Invalid parameters, out-of-range x1/x2 & malformed syntax guards", () => {
    std.testing.expectError(CssEasingError.InvalidEasing, EasingFunction.parse("")) catch unreachable;
    std.testing.expectError(CssEasingError.InvalidEasing, EasingFunction.parse("unknown-fn(1)")) catch unreachable;
    std.testing.expectError(CssEasingError.InvalidEasing, EasingFunction.parse("cubic-bezier(0.1, 0.2, 0.3)")) catch unreachable;

    // x1 < 0 or x2 > 1 forbidden by CSS Easing spec
    std.testing.expectError(CssEasingError.InvalidBezierControlPoint, EasingFunction.parse("cubic-bezier(-0.1, 0, 0.5, 1)")) catch unreachable;
    std.testing.expectError(CssEasingError.InvalidBezierControlPoint, EasingFunction.parse("cubic-bezier(0.1, 0, 1.2, 1)")) catch unreachable;

    // steps(0) and steps(1, jump-none) forbidden by CSS Easing spec
    std.testing.expectError(CssEasingError.InvalidStepCount, EasingFunction.parse("steps(0)")) catch unreachable;
    std.testing.expectError(CssEasingError.InvalidStepCount, EasingFunction.parse("steps(1, jump-none)")) catch unreachable;
    std.testing.expectError(CssEasingError.InvalidStepPosition, EasingFunction.parse("steps(4, middle)")) catch unreachable;

    // BufferTooSmall guard
    let tiny: [3]u8 = [0, 0, 0];
    std.testing.expectError(CssEasingError.BufferTooSmall, EasingFunction.linear().toCss(tiny[0..])) catch unreachable;
});
