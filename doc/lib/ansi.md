# `cap:ansi` — Unified ANSI Terminal Styling, SGR, Hyperlinks & Stripping

Zero-allocation pure-CP ANSI/VT100 terminal toolkit combining:
- **OSC 8 Hyperlinks** (`link.cp`): ST/BEL hyperlink encoding, zero-copy parsing, detection, and stripping.
- **SGR Style & Color Codec** (`sgr.cp`): 16-color, 256-color palette, and 24-bit TrueColor RGB formatting and atomic incremental SGR decoding.
- **ANSI Escape Stripper** (`strip.cp`): CSI/OSC/DCS/APC/PM/SOS/charset/2-byte ESC/C1 CSI stripping and visible length/codepoint counting.
- **16-Color Style Builder** (`style.cp`): ergonomic `AnsiColor` / `AnsiStyle` formatter and `visibleLen`.

## Exports

### OSC 8 Hyperlinks (`./link.cp`)
- `Osc8Link` (`url: []const u8`, `text: []const u8`, `params: []const u8`)
- `formatOsc8(url, text, params, out): ([]const u8)?` — ST (`\x1b\\`) hyperlink formatter
- `formatOsc8Bel(url, text, out): ([]const u8)?` — BEL (`\x07`) hyperlink formatter
- `parseOsc8(input): Osc8Link?` — Zero-copy single OSC 8 hyperlink parser (ST & BEL)
- `stripOsc8(input, out): ([]const u8)?` — Strips OSC 8 control sequences while preserving visible text
- `hasOsc8(input): bool` — Fast `\x1b]8;` scan
- `osc8EncodedLen(url, text, params): usize` / `osc8BelEncodedLen(url, text): usize`

### SGR Codec (`./sgr.cp`)
- `SgrColorKind` (`none`, `ansi16`, `ansi256`, `rgb`)
- `SgrColor` (`none()`, `ansi16(code)`, `ansi256(idx)`, `rgb(r, g, b)`, `isNone()`, `equals(other)`)
- `SgrStyle` (`plain()`, `reset(*_)`, `isPlain()`, `formatOpen(out)`)
- `formatStyled(text, style, out): ([]const u8)?`
- `applySgrSequence(seq, style: *SgrStyle): bool`

### Escape Stripper (`./strip.cp`)
- `hasAnsi(src): bool`
- `stripAnsi(src, out): ([]const u8)?`
- `strippedByteLen(src): usize`
- `visibleCodepointCount(src): usize`

### 16-Color Style Builder (`./style.cp`)
- `AnsiColor` (`black`..`white`, `bright_black`..`bright_white`, `fgCode()`, `bgCode()`)
- `AnsiStyle` (`isEmpty()`, `format(text, out)`, `formatRgb(r, g, b, text, out)`)
- `visibleLen(input): usize`

## Example

```cp
import {
    SgrColor,
    SgrStyle,
    formatStyled,
    formatOsc8,
    stripAnsi,
    hasAnsi,
} from "cap:ansi";

let buf: [256]u8 = [0; 256];
let style = SgrStyle.plain();
style.bold = true;
style.fg = SgrColor.rgb(80, 200, 120);

if (styled := formatStyled("Ready", style, buf[0..])) {
    let plain_buf: [64]u8 = [0; 64];
    * = stripAnsi(styled, plain_buf[0..]); // "Ready"
}
```
