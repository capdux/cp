# `cap:cli_spinner` — Zero-Alloc CLI Spinner Renderer

Zero-allocation pure-CP terminal spinner animation presets and status-line formatter supporting TTY line clearing (`\r\x1b[2K`) and non-TTY log output.

## Types

### `SpinnerPreset`

```cp
pub enum SpinnerPreset {
    dots,
    line,
    arc,
    arrows,
    bouncing_bar,
}
```

### `SpinnerSpec`

```cp
pub struct SpinnerSpec {
    preset: SpinnerPreset;
    interval_ms: u32;
    frame_count: usize;
}
```

- `SpinnerSpec.forPreset(preset: SpinnerPreset): SpinnerSpec` — Returns canonical interval and frame count (`dots`: 10 frames @ 80ms, `line`: 4 frames @ 130ms, `arc`: 6 frames @ 100ms, `arrows`: 8 frames @ 100ms, `bouncing_bar`: 6 frames @ 80ms).
- `frameAt(index: usize): []const u8` — Returns frame at `index % frame_count` (never out of bounds).
- `frameAtMs(elapsed_ms: u64): []const u8` — Returns active frame for `elapsed_ms / interval_ms` wrapped modulo `frame_count`.

## Exports

- `formatSpinnerFrame(preset: SpinnerPreset, elapsed_ms: u64, message: []const u8, is_tty: bool, out: []u8): ([]const u8)?` — Formats `\r\x1b[2K<frame> <message>` (TTY) or `<frame> <message>` (non-TTY) into `out`. Returns `null` if `out` is too small.
- `formatSpinnerDone(symbol: []const u8, message: []const u8, is_tty: bool, out: []u8): ([]const u8)?` — Formats `\r\x1b[2K<symbol> <message>\n` (TTY) or `<symbol> <message>\n` (non-TTY) into `out`. Returns `null` if `out` is too small.

## Example

```cp
import { SpinnerPreset, formatSpinnerFrame, formatSpinnerDone } from "cap:cli_spinner";

let buf: [128]u8 = [0; 128];
if (frame := formatSpinnerFrame(SpinnerPreset.dots, 160, "Installing...", true, buf[0..])) {
    // "\r\x1b[2K⠹ Installing..."
}
if (done := formatSpinnerDone("✓", "Installed 42 packages", true, buf[0..])) {
    // "\r\x1b[2K✓ Installed 42 packages\n"
}
```
