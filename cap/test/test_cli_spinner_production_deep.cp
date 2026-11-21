import {
    SpinnerPreset,
    SpinnerSpec,
    formatSpinnerFrame,
    formatSpinnerDone,
} from "cap:cli_spinner";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("SpinnerSpec.forPreset returns valid metadata and wraps frameAt safely", () => {
    const dots = SpinnerSpec.forPreset(SpinnerPreset.dots);
    expect(dots.interval_ms == 80 && dots.frame_count == 10);
    expect(strEq(dots.frameAt(0), "⠋"));
    expect(strEq(dots.frameAt(9), "⠏"));
    expect(strEq(dots.frameAt(10), "⠋"));
    expect(strEq(dots.frameAt(103), "⠸"));

    const line = SpinnerSpec.forPreset(SpinnerPreset.line);
    expect(line.interval_ms == 130 && line.frame_count == 4);
    expect(strEq(line.frameAt(0), "-"));
    expect(strEq(line.frameAt(1), "\\"));
    expect(strEq(line.frameAt(2), "|"));
    expect(strEq(line.frameAt(3), "/"));
    expect(strEq(line.frameAt(4), "-"));

    const arc = SpinnerSpec.forPreset(SpinnerPreset.arc);
    expect(arc.frame_count == 6 && strEq(arc.frameAt(0), "◜") && strEq(arc.frameAt(5), "◟"));

    const arrows = SpinnerSpec.forPreset(SpinnerPreset.arrows);
    expect(arrows.frame_count == 8 && strEq(arrows.frameAt(0), "←") && strEq(arrows.frameAt(4), "→"));

    const bar = SpinnerSpec.forPreset(SpinnerPreset.bouncing_bar);
    expect(bar.frame_count == 6 && strEq(bar.frameAt(0), "[=   ]") && strEq(bar.frameAt(5), "[   =]"));
});

test("SpinnerSpec.frameAtMs computes elapsed time stepping and huge u64 timestamps", () => {
    const dots = SpinnerSpec.forPreset(SpinnerPreset.dots);
    expect(strEq(dots.frameAtMs(0), "⠋"));
    expect(strEq(dots.frameAtMs(79), "⠋"));
    expect(strEq(dots.frameAtMs(80), "⠙"));
    expect(strEq(dots.frameAtMs(160), "⠹"));
    expect(strEq(dots.frameAtMs(800), "⠋")); // full 10-frame cycle

    const line = SpinnerSpec.forPreset(SpinnerPreset.line);
    expect(strEq(line.frameAtMs(0), "-"));
    expect(strEq(line.frameAtMs(130), "\\"));
    expect(strEq(line.frameAtMs(260), "|"));
    expect(strEq(line.frameAtMs(390), "/"));
    expect(strEq(line.frameAtMs(520), "-"));

    // Large timestamp wrap-around without overflow
    const huge: u64 = 1000000000000;
    const f = dots.frameAtMs(huge);
    expect(f.len > 0);
});

test("formatSpinnerFrame handles TTY vs non-TTY modes and empty messages", () => {
    let buf: [128]u8 = [0; 128];

    // TTY mode with clear-line escape prefix
    const tty_out = formatSpinnerFrame(SpinnerPreset.dots, 160, "Building...", true, buf[0..]).?;
    expect(strEq(tty_out, "\r\x1b[2K⠹ Building..."));

    // Non-TTY mode without escape codes
    const plain_out = formatSpinnerFrame(SpinnerPreset.line, 260, "Compiling", false, buf[0..]).?;
    expect(strEq(plain_out, "| Compiling"));

    // Empty message omits trailing space
    const no_msg_tty = formatSpinnerFrame(SpinnerPreset.bouncing_bar, 0, "", true, buf[0..]).?;
    expect(strEq(no_msg_tty, "\r\x1b[2K[=   ]"));

    const no_msg_plain = formatSpinnerFrame(SpinnerPreset.arc, 100, "", false, buf[0..]).?;
    expect(strEq(no_msg_plain, "◠"));
});

test("formatSpinnerDone formats success/error completion lines for TTY and non-TTY", () => {
    let buf: [128]u8 = [0; 128];

    const done_tty = formatSpinnerDone("✓", "Installed 12 packages", true, buf[0..]).?;
    expect(strEq(done_tty, "\r\x1b[2K✓ Installed 12 packages\n"));

    const done_plain = formatSpinnerDone("✗", "Build failed", false, buf[0..]).?;
    expect(strEq(done_plain, "✗ Build failed\n"));

    // Empty symbol or empty message edge cases
    const sym_only = formatSpinnerDone("OK", "", false, buf[0..]).?;
    expect(strEq(sym_only, "OK\n"));

    const msg_only = formatSpinnerDone("", "Done", true, buf[0..]).?;
    expect(strEq(msg_only, "\r\x1b[2KDone\n"));
});

test("buffer capacity boundary checks and defensive zero-field SpinnerSpec", () => {
    let buf: [32]u8 = [0; 32];

    // Non-TTY "| Go" = 4 bytes
    expect(strEq(formatSpinnerFrame(SpinnerPreset.line, 260, "Go", false, buf[0..4]).?, "| Go"));
    expect(formatSpinnerFrame(SpinnerPreset.line, 260, "Go", false, buf[0..3]) == null);

    // TTY "\r\x1b[2K- Hi" = 5 + 1 + 1 + 2 = 9 bytes
    expect(strEq(formatSpinnerFrame(SpinnerPreset.line, 0, "Hi", true, buf[0..9]).?, "\r\x1b[2K- Hi"));
    expect(formatSpinnerFrame(SpinnerPreset.line, 0, "Hi", true, buf[0..8]) == null);

    // Done "OK done\n" = 8 bytes
    expect(strEq(formatSpinnerDone("OK", "done", false, buf[0..8]).?, "OK done\n"));
    expect(formatSpinnerDone("OK", "done", false, buf[0..7]) == null);

    // Manually constructed zero-interval / zero-count spec must not divide by zero
    const safe_spec = SpinnerSpec { preset: SpinnerPreset.line, interval_ms: 0, frame_count: 0 };
    expect(strEq(safe_spec.frameAt(5), "-"));
    expect(strEq(safe_spec.frameAtMs(500), "-"));
});
