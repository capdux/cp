# @cap/ora-spinner

Clean-room MIT, zero-heap CLI terminal spinner animation and status line renderer for CP — modeled on npm `ora` v8.

## Dependencies
- `@cap/ansi-escapes` (`1.0.0`) — reused for `cursorHide`, `cursorShow`, `eraseLine`, and `cursorLeft` terminal escape sequences.

## API

### Style Constants
- `SPINNER_DOTS (0)` — 10 Braille dot frames (`⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏`)
- `SPINNER_LINE (1)` — 4 ASCII line frames (`-`, `\`, `|`, `/`)
- `SPINNER_ARC (2)` — 6 arc frames (`◜◠◝◞◡◟`)
- `SPINNER_BOUNCE (3)` — 4 bounce frames (`⠁⠂⠄⠂`)
- `SPINNER_ARROW (4)` — 8 directional arrow frames (`←↖↑↗→↘↓↙`)

### Struct `Spinner`
- `Spinner.new(style: u8, text: string): Spinner`
- `frameCount(): usize`
- `currentFrame(): string`
- `tick(*_): string` — advances frame index (wrapping around) and returns the new frame glyph.
- `setText(*_, text: string)` — updates the spinner message text.
- `renderFrame(with_ansi: bool, out: []u8): usize` — renders frame and text into `out`, prepending `CURSOR_LEFT + ERASE_LINE` when `with_ansi` is `true`.
- `renderSucceed(msg: string, out: []u8): usize` — renders `✔ <msg>\n`.
- `renderFail(msg: string, out: []u8): usize` — renders `✖ <msg>\n`.
- `renderWarn(msg: string, out: []u8): usize` — renders `⚠ <msg>\n`.
- `renderInfo(msg: string, out: []u8): usize` — renders `ℹ <msg>\n`.
- `hideCursorSeq(): string` — returns `CURSOR_HIDE`.
- `showCursorSeq(): string` — returns `CURSOR_SHOW`.
