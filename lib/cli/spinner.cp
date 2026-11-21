// Source: Absorbed from /data/dev/bun/src/cli/ | Synced: 2026-11-21T16:39:27Z (Zero-Alloc CLI Spinner Renderer)
// Pure-CP zero-allocation CLI terminal spinner frame presets and status-line renderer.

/// Built-in terminal spinner animation presets.
pub enum SpinnerPreset {
    dots,
    line,
    arc,
    arrows,
    bouncing_bar,
}

/// Static metadata and frame lookup for a spinner preset.
pub struct SpinnerSpec {
    preset: SpinnerPreset;
    interval_ms: u32;
    frame_count: usize;

    /// Return the canonical specification for a built-in spinner preset.
    static fn forPreset(preset: SpinnerPreset): SpinnerSpec {
        if (preset == SpinnerPreset.dots) {
            return SpinnerSpec { preset: preset, interval_ms: 80, frame_count: 10 };
        }
        if (preset == SpinnerPreset.line) {
            return SpinnerSpec { preset: preset, interval_ms: 130, frame_count: 4 };
        }
        if (preset == SpinnerPreset.arc) {
            return SpinnerSpec { preset: preset, interval_ms: 100, frame_count: 6 };
        }
        if (preset == SpinnerPreset.arrows) {
            return SpinnerSpec { preset: preset, interval_ms: 100, frame_count: 8 };
        }
        // SpinnerPreset.bouncing_bar
        return SpinnerSpec { preset: preset, interval_ms: 80, frame_count: 6 };
    }

    /// Return the frame string at `index` (automatically wrapped modulo `frame_count`).
    fn frameAt(index: usize): []const u8 {
        const count = if (_.frame_count > 0) _.frame_count else 1;
        return presetFrame(_.preset, index % count);
    }

    /// Return the active frame string for `elapsed_ms` based on `interval_ms`.
    fn frameAtMs(elapsed_ms: u64): []const u8 {
        const iv: u64 = if (_.interval_ms > 0) (_.interval_ms as u64) else 1;
        const count: u64 = if (_.frame_count > 0) (_.frame_count as u64) else 1;
        const idx: usize = ((elapsed_ms / iv) % count) as usize;
        return presetFrame(_.preset, idx);
    }
}

/// Lookup a single frame slice for `preset` at normalized `idx`.
fn presetFrame(preset: SpinnerPreset, idx: usize): []const u8 {
    if (preset == SpinnerPreset.dots) {
        const frames: [10][]const u8 = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"];
        return frames[idx % 10];
    }
    if (preset == SpinnerPreset.line) {
        const frames: [4][]const u8 = ["-", "\\", "|", "/"];
        return frames[idx % 4];
    }
    if (preset == SpinnerPreset.arc) {
        const frames: [6][]const u8 = ["◜", "◠", "◝", "◞", "◡", "◟"];
        return frames[idx % 6];
    }
    if (preset == SpinnerPreset.arrows) {
        const frames: [8][]const u8 = ["←", "↖", "↑", "↗", "→", "↘", "↓", "↙"];
        return frames[idx % 8];
    }
    const frames: [6][]const u8 = ["[=   ]", "[==  ]", "[=== ]", "[ ===]", "[  ==]", "[   =]"];
    return frames[idx % 6];
}

/// Append `src` into `out` at `offset`, returning new offset or `null` if `out` is too small.
fn pushBytes(out: []u8, offset: usize, src: []const u8): usize? {
    if (offset + src.len > out.len) return null;
    let i: usize = 0;
    while (i < src.len) {
        out[offset + i] = src[i];
        i += 1;
    }
    return offset + src.len;
}

/// Format a spinner status frame into `out`.
/// - TTY mode (`is_tty == true`): `\r\x1b[2K<frame> <message>` (or `\r\x1b[2K<frame>` if message is empty)
/// - Non-TTY mode (`is_tty == false`): `<frame> <message>` (or `<frame>` if message is empty)
/// Returns `null` if `out` is too small.
pub fn formatSpinnerFrame(
    preset: SpinnerPreset,
    elapsed_ms: u64,
    message: []const u8,
    is_tty: bool,
    out: []u8,
): ([]const u8)? {
    const spec = SpinnerSpec.forPreset(preset);
    const frame = spec.frameAtMs(elapsed_ms);

    let w: usize = 0;
    if (is_tty) {
        w = pushBytes(out, w, "\r\x1b[2K") orelse return null;
    }
    w = pushBytes(out, w, frame) orelse return null;
    if (message.len > 0) {
        w = pushBytes(out, w, " ") orelse return null;
        w = pushBytes(out, w, message) orelse return null;
    }
    return out[0..w];
}

/// Format a completed spinner status line into `out`.
/// - TTY mode (`is_tty == true`): `\r\x1b[2K<symbol> <message>\n`
/// - Non-TTY mode (`is_tty == false`): `<symbol> <message>\n`
/// Returns `null` if `out` is too small.
pub fn formatSpinnerDone(
    symbol: []const u8,
    message: []const u8,
    is_tty: bool,
    out: []u8,
): ([]const u8)? {
    let w: usize = 0;
    if (is_tty) {
        w = pushBytes(out, w, "\r\x1b[2K") orelse return null;
    }
    if (symbol.len > 0) {
        w = pushBytes(out, w, symbol) orelse return null;
        if (message.len > 0) {
            w = pushBytes(out, w, " ") orelse return null;
        }
    }
    if (message.len > 0) {
        w = pushBytes(out, w, message) orelse return null;
    }
    w = pushBytes(out, w, "\n") orelse return null;
    return out[0..w];
}
