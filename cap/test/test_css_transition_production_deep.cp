import { test, expect_eq } from "cap:test";
import {
    CssTransitionError,
    TransitionBehavior,
    TransitionPropertyKind,
    TransitionTime,
    TransitionEasingKind,
    TransitionEasing,
    CssTransition as Transition,
    TransitionList,
} from "cap:css";

test("1. TransitionTime (s/ms) canonical shortest serialization & TransitionBehavior", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);

    const t0 = TransitionTime.parse("0ms", false) catch unreachable;
    expect_eq(t0.isZero(), true);
    const n0 = t0.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n0], "0s"), true);

    const t200 = TransitionTime.parse("0.2s", false) catch unreachable;
    expect_eq(t200.ms, 200.0);
    const n200 = t200.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n200], "200ms"), true);

    const t1500 = TransitionTime.parse("1500ms", false) catch unreachable;
    expect_eq(t1500.seconds(), 1.5);
    const n1500 = t1500.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n1500], "1.5s"), true);

    expect_eq(TransitionBehavior.parse("allow-discrete").? == TransitionBehavior.allow_discrete, true);
});

test("2. Order-independent Transition parse & dual-time duration/delay rule", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    // Easing first, duration, property, negative delay, allow-discrete
    const tr = Transition.parse("ease-out 300ms opacity -50ms allow-discrete") catch unreachable;
    expect_eq(tr.property.kind == TransitionPropertyKind.custom_ident, true);
    expect_eq(std.mem.eql(u8, tr.property.name, "opacity"), true);
    expect_eq(tr.duration.ms, 300.0);
    expect_eq(tr.delay.ms, -50.0);
    expect_eq(tr.timing_function.kind == TransitionEasingKind.ease_out, true);
    expect_eq(tr.behavior == TransitionBehavior.allow_discrete, true);

    const n = tr.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n], "opacity 300ms ease-out -50ms allow-discrete"), true);

    // Zero duration with non-zero delay MUST still emit duration (`0s`) before delay
    const tr_delay = Transition.parse("color 0s 100ms") catch unreachable;
    const nd = tr_delay.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..nd], "color 0s 100ms"), true);
});

test("3. Default omission & cubic-bezier / steps easing inside Transition", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    // Default duration=0s, delay=0s, easing=ease, behavior=normal -> just "all"
    const tr_def = Transition.parse("all 0s ease 0ms normal") catch unreachable;
    const nd = tr_def.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..nd], "all"), true);

    const tr_cb = Transition.parse("transform 0.5s cubic-bezier(0.2, 0, 0, 1)") catch unreachable;
    expect_eq(tr_cb.timing_function.kind == TransitionEasingKind.cubic_bezier, true);
    const ncb = tr_cb.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..ncb], "transform 500ms cubic-bezier(0.2,0,0,1)"), true);

    const tr_st = Transition.parse("width 1s steps(4, jump-start)") catch unreachable;
    expect_eq(tr_st.timing_function.kind == TransitionEasingKind.steps, true);
    const nst = tr_st.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..nst], "width 1s steps(4,start)"), true);
});

test("4. Multi-transition TransitionList preserving commas inside cubic-bezier/steps", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);

    const list = TransitionList.parse("opacity 200ms ease-in, transform 1.5s cubic-bezier(0.1, 0.7, 1, 0.1) 50ms") catch unreachable;
    expect_eq(list.len, usize(2));
    expect_eq(std.mem.eql(u8, list.items[0].property.name, "opacity"), true);
    expect_eq(std.mem.eql(u8, list.items[1].property.name, "transform"), true);
    expect_eq(list.items[1].delay.ms, 50.0);

    const n = list.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n], "opacity 200ms ease-in, transform 1.5s cubic-bezier(0.1,0.7,1,0.1) 50ms"), true);
});

test("5. Invalid inputs (negative duration, duplicate property, out-of-range cubic-bezier x)", () => {
    // Negative duration forbidden
    std.testing.expectError(CssTransitionError.InvalidDuration, Transition.parse("opacity -200ms")) catch unreachable;

    // Duplicate property forbidden
    std.testing.expectError(CssTransitionError.InvalidTransition, Transition.parse("opacity transform 200ms")) catch unreachable;

    // Third time value forbidden
    std.testing.expectError(CssTransitionError.InvalidTransition, Transition.parse("opacity 100ms 50ms 20ms")) catch unreachable;

    // Out-of-range cubic-bezier x1/x2
    std.testing.expectError(CssTransitionError.InvalidEasing, TransitionEasing.parse("cubic-bezier(-0.2, 0, 1, 1)")) catch unreachable;
    std.testing.expectError(CssTransitionError.InvalidEasing, TransitionEasing.parse("cubic-bezier(0.2, 0, 1.5, 1)")) catch unreachable;

    // Empty / trailing comma
    std.testing.expectError(CssTransitionError.InvalidTransition, TransitionList.parse("")) catch unreachable;
    std.testing.expectError(CssTransitionError.InvalidTransition, TransitionList.parse("opacity 1s, ")) catch unreachable;
});
