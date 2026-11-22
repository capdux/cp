// Source: cp/cap/test/test_tween_production_deep.cp
// Deep Production Verification Suite for cap:tween & cap.tween
// Covers:
//   1. All 31 EaseKind curves: strict endpoints (0.0/1.0), monotonicity, InBack undershoot, OutBack/OutElastic overshoot, OutBounce rebounds, smoothstep & lerp
//   2. Tween forward pass, yoyo oscillation with finite repeat_count, reset, and InvalidDuration guard
//   3. KeyframeTrack piecewise eased interpolation, out-of-range clamping, EmptyTrack / InvalidKeyframeOrder guards, and zero-leak allocator verification
//   4. SpringDamper underdamped overshoot convergence, critically damped zero-overshoot convergence, large-dt stability, InvalidSpringParams guard
//   5. `import cap from "cap"; cap.tween` dynamic lazy namespace mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TweenError,
    EaseKind,
    Easing,
    Tween,
    Keyframe,
    KeyframeTrack,
    SpringDamper,
} from "cap:time";

fn expectNear(actual: f64, expected: f64, tol: f64) {
    expect_eq(@abs(actual - expected) <= tol, true);
}

test("cap:tween - EaseKind endpoints, monotonicity, and Back/Bounce/Elastic traits") {
    const tol: f64 = 0.000000001;

    const all_kinds = [
        EaseKind.Linear,
        EaseKind.InQuad,
        EaseKind.OutQuad,
        EaseKind.InOutQuad,
        EaseKind.InCubic,
        EaseKind.OutCubic,
        EaseKind.InOutCubic,
        EaseKind.InQuart,
        EaseKind.OutQuart,
        EaseKind.InOutQuart,
        EaseKind.InQuint,
        EaseKind.OutQuint,
        EaseKind.InOutQuint,
        EaseKind.InSine,
        EaseKind.OutSine,
        EaseKind.InOutSine,
        EaseKind.InExpo,
        EaseKind.OutExpo,
        EaseKind.InOutExpo,
        EaseKind.InCirc,
        EaseKind.OutCirc,
        EaseKind.InOutCirc,
        EaseKind.InBack,
        EaseKind.OutBack,
        EaseKind.InOutBack,
        EaseKind.InElastic,
        EaseKind.OutElastic,
        EaseKind.InOutElastic,
        EaseKind.InBounce,
        EaseKind.OutBounce,
        EaseKind.InOutBounce,
    ];

    // Strict endpoint & clamping checks for every EaseKind
    for (kind of all_kinds) {
        expectNear(Easing.sample(kind, -0.5), 0.0, tol);
        expectNear(Easing.sample(kind, 0.0), 0.0, tol);
        expectNear(Easing.sample(kind, 1.0), 1.0, tol);
        expectNear(Easing.sample(kind, 1.5), 1.0, tol);
    }

    // Monotonicity check on standard non-overshooting curves
    const mono_kinds = [
        EaseKind.Linear,
        EaseKind.InQuad,
        EaseKind.OutQuad,
        EaseKind.InOutQuad,
        EaseKind.InCubic,
        EaseKind.OutCubic,
        EaseKind.InOutCubic,
        EaseKind.InQuart,
        EaseKind.OutQuart,
        EaseKind.InOutQuart,
        EaseKind.InQuint,
        EaseKind.OutQuint,
        EaseKind.InOutQuint,
        EaseKind.InSine,
        EaseKind.OutSine,
        EaseKind.InOutSine,
        EaseKind.InExpo,
        EaseKind.OutExpo,
        EaseKind.InOutExpo,
        EaseKind.InCirc,
        EaseKind.OutCirc,
        EaseKind.InOutCirc,
    ];

    for (kind of mono_kinds) {
        let prev = Easing.sample(kind, 0.0);
        for (step of 1..=20) {
            const t = (step as f64) / 20.0;
            const cur = Easing.sample(kind, t);
            expect_eq(cur >= prev - tol, true);
            expect_eq(cur >= -tol && cur <= 1.0 + tol, true);
            prev = cur;
        }
    }

    // InBack undershoots below 0.0 early; OutBack overshoots above 1.0 late
    expect_eq(Easing.sample(EaseKind.InBack, 0.25) < 0.0, true);
    expect_eq(Easing.sample(EaseKind.OutBack, 0.75) > 1.0, true);

    // OutElastic overshoots above 1.0 near t = 0.2..0.3
    expect_eq(Easing.sample(EaseKind.OutElastic, 0.25) > 1.0, true);

    // OutBounce hits 1.0 at bounce peaks and dips in between, staying within [0, 1]
    const b1 = Easing.sample(EaseKind.OutBounce, 1.0 / 2.75);
    const b2 = Easing.sample(EaseKind.OutBounce, 1.5 / 2.75);
    expectNear(b1, 1.0, 0.0001);
    expect_eq(b2 < 1.0 && b2 >= 0.74, true);

    // Helper utilities: clamp01, lerp, lerpEased, smoothstep
    expectNear(Easing.clamp01(-2.0), 0.0, tol);
    expectNear(Easing.clamp01(0.42), 0.42, tol);
    expectNear(Easing.clamp01(3.0), 1.0, tol);
    expectNear(Easing.lerp(10.0, 20.0, 0.25), 12.5, tol);
    expectNear(Easing.lerpEased(0.0, 100.0, 0.5, EaseKind.InQuad), 25.0, tol);
    expectNear(Easing.smoothstep(10.0, 20.0, 10.0), 0.0, tol);
    expectNear(Easing.smoothstep(10.0, 20.0, 15.0), 0.5, tol);
    expectNear(Easing.smoothstep(10.0, 20.0, 20.0), 1.0, tol);
}

test("cap:tween - Tween forward, yoyo, repeat_count, and InvalidDuration") {
    const tol: f64 = 0.000000001;

    // InvalidDuration guard
    const bad_zero = Tween.init(0.0, EaseKind.Linear, false, 0);
    expect_eq(bad_zero == TweenError.InvalidDuration, true);
    const bad_neg = Tween.init(-50.0, EaseKind.Linear, false, 0);
    expect_eq(bad_neg == TweenError.InvalidDuration, true);

    // Single forward pass (repeat_count = 0)
    let tw = Tween.init(100.0, EaseKind.Linear, false, 0) catch unreachable;
    expect_eq(tw.isFinished(), false);
    expectNear(tw.update(25.0), 0.25, tol);
    expect_eq(tw.isFinished(), false);
    expectNear(tw.update(50.0), 0.75, tol);
    expect_eq(tw.isFinished(), false);
    expectNear(tw.update(40.0), 1.0, tol);
    expect_eq(tw.isFinished(), true);
    // Subsequent updates remain clamped at 1.0
    expectNear(tw.update(20.0), 1.0, tol);

    // Reset works cleanly
    tw.reset();
    expect_eq(tw.isFinished(), false);
    expectNear(tw.progress(), 0.0, tol);

    // Yoyo with repeat_count = 1 (pass 0: 0 -> 1, pass 1: 1 -> 0)
    let yoyo_tw = Tween.init(100.0, EaseKind.Linear, true, 1) catch unreachable;
    expectNear(yoyo_tw.update(50.0), 0.5, tol);
    expect_eq(yoyo_tw.isFinished(), false);
    // Crosses into pass 1 (reverse) at elapsed 25ms -> t = 1 - 0.25 = 0.75
    expectNear(yoyo_tw.update(75.0), 0.75, tol);
    expect_eq(yoyo_tw.isFinished(), false);
    // Completes pass 1 -> finishes at 0.0
    expectNear(yoyo_tw.update(100.0), 0.0, tol);
    expect_eq(yoyo_tw.isFinished(), true);
}

test("cap:tween - KeyframeTrack piecewise easing, clamping, and error guards") {
    const tol: f64 = 0.000000001;
    const alloc = std.testing.allocator;

    let track = KeyframeTrack.init(alloc);
    defer track.deinit();

    // EmptyTrack guard
    const empty_res = track.evaluate(0.5);
    expect_eq(empty_res == TweenError.EmptyTrack, true);

    // Add keyframes with increasing timestamps
    track.addKeyframe(1.0, 10.0, EaseKind.Linear) catch unreachable;
    // Single-keyframe evaluation returns its value everywhere
    expectNear(track.evaluate(0.0) catch unreachable, 10.0, tol);
    expectNear(track.evaluate(5.0) catch unreachable, 10.0, tol);

    // Non-monotonic timestamp guard (equal or smaller)
    expect_eq(track.addKeyframe(1.0, 20.0, EaseKind.Linear) == TweenError.InvalidKeyframeOrder, true);
    expect_eq(track.addKeyframe(0.5, 20.0, EaseKind.Linear) == TweenError.InvalidKeyframeOrder, true);

    track.addKeyframe(3.0, 50.0, EaseKind.InQuad) catch unreachable;
    track.addKeyframe(5.0, 150.0, EaseKind.Linear) catch unreachable;
    expect_eq(track.len(), 3);

    // Out-of-range clamping
    expectNear(track.evaluate(-10.0) catch unreachable, 10.0, tol);
    expectNear(track.evaluate(1.0) catch unreachable, 10.0, tol);
    expectNear(track.evaluate(5.0) catch unreachable, 150.0, tol);
    expectNear(track.evaluate(99.0) catch unreachable, 150.0, tol);

    // Segment 0 [1.0, 3.0] Linear: at t=2.0 (local_t=0.5) -> 30.0
    expectNear(track.evaluate(2.0) catch unreachable, 30.0, tol);

    // Segment 1 [3.0, 5.0] InQuad: at t=4.0 (local_t=0.5, eased=0.25) -> 50 + 100*0.25 = 75.0
    expectNear(track.evaluate(4.0) catch unreachable, 75.0, tol);
}

test("cap:tween - SpringDamper underdamped, critically damped, large dt stability & params guard") {
    const tol: f64 = 0.001;

    // InvalidSpringParams guards
    expect_eq(SpringDamper.init(0.0, 1.0, 0.0) == TweenError.InvalidSpringParams, true);
    expect_eq(SpringDamper.init(-10.0, 1.0, 0.0) == TweenError.InvalidSpringParams, true);
    expect_eq(SpringDamper.init(100.0, -0.1, 0.0) == TweenError.InvalidSpringParams, true);

    // 1. Underdamped spring (zeta = 0.25): must overshoot target=100.0 and eventually settle
    let under = SpringDamper.initWithRatio(10.0, 0.25, 0.0) catch unreachable;
    under.setTarget(100.0);
    let max_val: f64 = 0.0;
    for (_i of 0..300) {
        under.step(0.016);
        if (under.value() > max_val) {
            max_val = under.value();
        }
    }
    expect_eq(max_val > 110.0, true);
    expect_eq(under.isSettled(0.05, 0.05), true);
    expectNear(under.value(), 100.0, 0.05);

    // 2. Critically damped spring (k=100, c=2*sqrt(100)=20 -> zeta=1.0): zero overshoot
    let crit = SpringDamper.init(100.0, 20.0, 0.0) catch unreachable;
    expectNear(crit.naturalFrequency(), 10.0, 0.000001);
    expectNear(crit.dampingRatio(), 1.0, 0.000001);
    crit.setTarget(100.0);
    for (_i of 0..150) {
        crit.step(0.016);
        expect_eq(crit.value() <= 100.0 + tol, true);
    }
    expect_eq(crit.isSettled(0.01, 0.01), true);

    // 3. Large dt stability check (sub-stepping prevents explosion on 2.0s single step)
    let big_dt = SpringDamper.init(400.0, 20.0, 0.0) catch unreachable;
    big_dt.setTarget(50.0);
    big_dt.step(2.0);
    expect_eq(std.math.isFinite(big_dt.value()), true);
    expect_eq(std.math.isFinite(big_dt.velocity()), true);
    expectNear(big_dt.value(), 50.0, 0.01);
}

test("cap:tween - import cap from 'cap'; cap.tween dynamic lazy mounting") {
    const tol: f64 = 0.000000001;

    const eased = cap.time.Easing.lerpEased(0.0, 200.0, 0.5, cap.time.EaseKind.OutQuad);
    expectNear(eased, 150.0, tol);

    let tw = cap.time.Tween.init(100.0, cap.time.EaseKind.Linear, false, 0) catch unreachable;
    expectNear(tw.update(50.0), 0.5, tol);
    expect_eq(tw.isFinished(), false);
}
