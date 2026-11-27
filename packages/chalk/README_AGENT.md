# chalk@1.0.0 — AI Agent Quick Ref

- **Import**: `import { Chalk } from "chalk";`
- **Provenance**: npm `chalk` (v5) parity | Zero-heap | License: MIT
- **Deps**: `ansi-styles`, `supports-color`
- **Types**: `src/index.d.cp`

## Color Levels
- `0` (`COLOR_LEVEL_NONE`): plain text, no ANSI escapes
- `1` (`COLOR_LEVEL_BASIC`): 16-color basic ANSI
- `2` (`COLOR_LEVEL_256`): 256-color ANSI (`38;5;n`)
- `3` (`COLOR_LEVEL_16M`): 24-bit Truecolor (`38;2;r;g;b`)

## Core Class
```cp
export class Chalk {
    level: u8,
    fn new(level: u8): Chalk;
    fn fromEnv(env: TerminalEnv, argv: &[]string): Chalk;

    // Foreground (9): black, red, green, yellow, blue, magenta, cyan, white, gray
    fn red(text: string, out: []u8): usize;
    // Background (9): bgBlack, bgRed, bgGreen, bgYellow, bgBlue, bgMagenta, bgCyan, bgWhite, bgGray
    fn bgRed(text: string, out: []u8): usize;
    // Modifiers (6): bold, dim, italic, underline, inverse, strikethrough
    fn bold(text: string, out: []u8): usize;

    // Extended colors (auto-downsampled by level)
    fn ansi256(code: u8, text: string, out: []u8): usize;
    fn bgAnsi256(code: u8, text: string, out: []u8): usize;
    fn rgb(r: u8, g: u8, b: u8, text: string, out: []u8): usize;
    fn bgRgb(r: u8, g: u8, b: u8, text: string, out: []u8): usize;
    fn hex(hex_str: string, text: string, out: []u8): usize;
    fn bgHex(hex_str: string, text: string, out: []u8): usize;
    fn keyword(name: string, text: string, out: []u8): usize;
    fn bgKeyword(name: string, text: string, out: []u8): usize;

    // Combined style in one pass (StylePair imported from "ansi-styles")
    fn styled(text: string, is_bold: bool, is_underline: bool, fg: StylePair, out: []u8): usize;
}
```

## Minimal Example
```cp
import { Chalk } from "chalk";
const c = Chalk.new(3);
let buf: [128]u8 = undefined;
const n = c.rgb(255, 136, 0, "warn", buf[0..]); // buf[0..n]
```

## Gotchas
1. Every styling method takes a caller-owned `out: []u8` and returns bytes written; reserve at least `text.len + 32` bytes (or `text.len + 64` for `styled()`).
2. `Chalk.fromEnv(env, argv)` requires `TerminalEnv` from `"supports-color"`; for fixed color mode use `Chalk.new(0..3)`.
3. `styled()` takes a `StylePair` (e.g., `FG_RED`, `FG_CYAN`) imported from `"ansi-styles"`.
4. Invalid hex/keyword strings fall back safely without panicking.
