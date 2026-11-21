# `cap:cli_color_detect` — Terminal ANSI Color Capability Detector

Deterministic, zero-allocation terminal color capability detection following `no-color.org`, `force-color.org`, `CLICOLOR`, `COLORTERM`, `TERM`, `TERM_PROGRAM`, `WT_SESSION`, and `CI` conventions.

## Exports

- `ColorLevel` (`none` | `basic16` | `ansi256` | `truecolor`)
- `ColorEnv` — Pure input struct (`ColorEnv.empty(is_tty)`).
- `parseForceColor(val): ColorLevel?`
- `detectColorLevel(env: ColorEnv): ColorLevel`
- `supportsColor(env: ColorEnv): bool` / `supportsTruecolor(env: ColorEnv): bool`
