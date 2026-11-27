# `picocolors` — AI Agent Quick Reference

- **Package**: `picocolors` (`1.0.0`, MIT)
- **Origin**: `npm:picocolors` (built on `ansi-styles`)
- **Import**: `import { detectColorSupport, ColorConfig, PicoFormatter, PicoBuf, picoFormatPair } from "picocolors";`

## Types & Constructors

```cp
fn detectColorSupport(no_color: string, force_color: string, is_tty: bool): bool

class ColorConfig {
    is_color_supported: bool,
    fn enabled(): ColorConfig,
    fn disabled(): ColorConfig,
    fn fromEnv(no_color: string, force_color: string, is_tty: bool): ColorConfig,
}
```

## `PicoFormatter` (writes single styled string into `out: []u8`, returns `usize`)

```cp
// Constructors & meta: new(bool), fromConfig(ColorConfig), isColorSupported(): bool
// Generic: style(pair: StylePair, text: string, out: []u8): usize
// Modifiers: reset, bold, dim, italic, underline, inverse, hidden, strikethrough
// Foregrounds: black, red, green, yellow, blue, magenta, cyan, white, gray
// Backgrounds: bgBlack, bgRed, bgGreen, bgYellow, bgBlue, bgMagenta, bgCyan, bgWhite
// All style methods have signature: (text: string, out: []u8): usize
```

## `PicoBuf` (1024-byte stack builder, appends in-place, returns bytes written `usize`)

```cp
// Fields: cfg: ColorConfig, buf: [1024]u8, len: usize
// Constructors & control: new(bool), fromConfig(ColorConfig), clear()
// Generic: pushPlain(text: string): usize, pushStyled(pair: StylePair, text: string): usize
// Shorthands: bold, dim, italic, underline, inverse, strikethrough,
//   black, red, green, yellow, blue, magenta, cyan, white, gray,
//   bgRed, bgGreen, bgYellow, bgBlue
```

## Minimal Pattern

```cp
let pb = PicoBuf.fromConfig(ColorConfig.fromEnv("", "", true));
* = pb.green("OK");
* = pb.pushPlain(" done");
let line = pb.buf[0..pb.len] as string;
```

## Pitfalls

1. **Nested close-code recovery**: `PicoFormatter` / `picoFormatPair` automatically replaces embedded close codes with open codes; prefer `PicoFormatter` when nesting styles.
2. **Missing `PicoBuf` shorthands**: `bgBlack`, `bgMagenta`, `bgCyan`, `bgWhite`, `hidden`, `reset` are on `PicoFormatter`, or pass `ansi-styles` pairs to `pb.pushStyled(pair, text)`.
3. **1024-byte cap on `PicoBuf`**: Call `pb.clear()` between lines; for longer output use `PicoFormatter` with a larger `out` slice.
4. **Plain fallback**: When `is_color_supported == false`, output is unstyled plain text.
