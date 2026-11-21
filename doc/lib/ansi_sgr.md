# `cap:ansi_sgr` — ANSI SGR Style & Color Codec

Zero-allocation pure-CP encoder and stateful incremental decoder for ANSI SGR (Select Graphic Rendition) escape sequences, supporting text attributes, 16-color, 256-color palette, and 24-bit TrueColor RGB.

## Types

### `SgrColorKind`

```cp
pub enum SgrColorKind {
    none,
    ansi16,
    ansi256,
    rgb,
}
```

### `SgrColor`

```cp
pub struct SgrColor {
    kind: SgrColorKind;
    code: u8;
    r: u8;
    g: u8;
    b: u8;
}
```

Factory & helper methods:
- `SgrColor.none(): SgrColor`
- `SgrColor.ansi16(code: u8): SgrColor` (accepts both 0..15 palette indices and raw SGR codes `30..37`/`90..97`/`40..47`/`100..107`)
- `SgrColor.ansi256(idx: u8): SgrColor`
- `SgrColor.rgb(r: u8, g: u8, b: u8): SgrColor`
- `isNone(): bool` / `equals(other: SgrColor): bool`

### `SgrStyle`

```cp
pub struct SgrStyle {
    bold: bool;
    dim: bool;
    italic: bool;
    underline: bool;
    inverse: bool;
    strikethrough: bool;
    fg: SgrColor;
    bg: SgrColor;
}
```

Methods:
- `SgrStyle.plain(): SgrStyle`
- `reset(*_)` — Clears all attributes and colors to plain.
- `isPlain(): bool` — Returns `true` when no attributes or colors are set.
- `formatOpen(out: []u8): ([]const u8)?` — Formats `\x1b[...m` into `out` (returns empty slice when plain, or `null` if `out` is too small).

## Exports

- `formatStyled(text: []const u8, style: SgrStyle, out: []u8): ([]const u8)?` — Formats `<open><text>\x1b[0m` into `out` (or copies `text` directly when `style.isPlain()`). Returns `null` if `out` is too small.
- `applySgrSequence(seq: []const u8, style: *SgrStyle): bool` — Parses a single `\x1b[...m` sequence and atomically applies incremental updates to `style` (`0`/empty reset, `1/2/3/4/7/9` on, `22/23/24/27/29` off, `30..37`/`90..97` fg, `40..47`/`100..107` bg, `39`/`49` default, `38;5;n`/`48;5;n`, `38;2;r;g;b`/`48;2;r;g;b`). Returns `false` without mutating `style` on malformed input.

## Example

```cp
import { SgrColor, SgrStyle, formatStyled, applySgrSequence } from "cap:ansi_sgr";

let style = SgrStyle.plain();
style.bold = true;
style.fg = SgrColor.rgb(255, 128, 0);

let buf: [128]u8 = [0; 128];
if (rendered := formatStyled("Warning", style, buf[0..])) {
    // rendered == "\x1b[1;38;2;255;128;0mWarning\x1b[0m"
}

let parsed = SgrStyle.plain();
* = applySgrSequence("\x1b[1;38;5;214m", &parsed);
```
