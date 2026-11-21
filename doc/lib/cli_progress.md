# `cap:cli_progress` — CLI Progress Bar & Spinner Renderer

Zero-allocation CLI progress bar, completion percentage, pure-integer ETA estimator, and Braille/ASCII spinner frame renderer.

## Exports

- `SpinnerStyle` (`ascii` | `dots` | `line`) & `spinnerFrame(style, tick): []const u8`
- `ProgressBar` — `init(total, width)`, `set(*_, value)`, `inc(*_, delta)`, `percent(): u8`, `isComplete(): bool`, `etaMs(elapsed_ms): u64?`, `render(out): ([]const u8)?`.
