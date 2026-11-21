import { test, expect } from "cap:test";
import {
    AnimationDirection,
    AnimationPlayState,
    AnimationFillMode,
    IterationCount,
    Animation,
    Transition,
} from "cap:css_animation";

test("css_animation: Animation shorthand parse and canonical shortest toCss", () => {
    const a = Animation.parse("300ms ease-in 50ms infinite alternate forwards paused fadeIn").?;
    expect(a.name).toBe("fadeIn");
    expect(a.duration.toMs()).toBe(300.0);
    expect(a.delay.toMs()).toBe(50.0);
    expect(a.iteration_count.infinite).toBeTrue();
    expect(a.direction == .alternate).toBeTrue();
    expect(a.fill_mode == .forwards).toBeTrue();
    expect(a.play_state == .paused).toBeTrue();

    let buf: [128]u8 = undefined;
    expect(a.toCss(buf[0..])).toBe("0.3s ease-in 50ms infinite alternate forwards paused fadeIn");

    // Default values elided
    const simple = Animation.parse("1s ease 0s 1 normal none running slide").?;
    expect(simple.toCss(buf[0..])).toBe("1s slide");
});

test("css_animation: Transition shorthand with cubic-bezier and error rejection", () => {
    const t = Transition.parse("opacity 250ms cubic-bezier(0.25, 0.1, 0.25, 1) 100ms").?;
    expect(t.property).toBe("opacity");
    expect(t.duration.toMs()).toBe(250.0);
    expect(t.delay.toMs()).toBe(100.0);
    expect(t.timing_function.isEase()).toBeTrue();

    let buf: [128]u8 = undefined;
    // cubic-bezier(0.25, 0.1, 0.25, 1) == ease (default), so it is omitted!
    expect(t.toCss(buf[0..])).toBe("opacity 0.25s 0.1s");

    expect(Transition.parse("") == null).toBeTrue();
    expect(Animation.parse("") == null).toBeTrue();
});
