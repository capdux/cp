# @cap/css-color

Zero-allocation CSS Color Level 4 functional color parser, formatter, and RGB <-> HSL converter for CP.

## Features

- **Zero heap allocation**: parses and formats directly over `string` / `[]u8` buffers.
- **CSS Color 4 functional syntax**: supports `rgb(...)`, `rgba(...)`, `hsl(...)`, `hsla(...)` in both legacy comma syntax and modern space + `/` alpha syntax (including `%` alpha and `deg` angles).
- **Color space conversion**: accurate `hslToRgb` and `rgbToHsl`.
- **Canonical formatting**: `formatRgb` emits `rgb(r, g, b)` for opaque colors (`a >= 0.999`) and `rgba(r, g, b, a)` for translucent colors.

## Usage

```cp
import { parseCssColor, formatRgb, rgbToHsl } from "@cap/css-color";

const c = parseCssColor("rgb(255 0 128 / 50%)");
let buf: [32]u8 = undefined;
const n = formatRgb(c, buf[0..32]); // "rgba(255, 0, 128, 0.5)"
```
