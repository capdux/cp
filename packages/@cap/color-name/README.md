<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`color-name`](https://github.com/colorjs/color-name)（`MIT`）（包名 `@cap/color-name`）。

# `@cap/color-name`

Zero-heap-allocation W3C CSS Color Level 4 named color table (all 148 standard colors) in pure CP (clean-room MIT implementation inspired by npm `color-name`).

## Features

- **`CSS_COLOR_COUNT` (`148`)** — Complete W3C CSS4 color set from `aliceblue` to `yellowgreen`, including `rebeccapurple`, `gray`/`grey` spellings, `aqua`/`cyan`, and `fuchsia`/`magenta`.
- **`lookupColor(name)`** — Case-insensitive O(log n) binary search returning `RgbColor { r, g, b, found }`.
- **`lookupColorHex(name)`** — Returns packed `0xRRGGBB` (`i32`) or `-1` if unknown.
- **`reverseLookupColor(r, g, b)`** — Exact RGB-to-name reverse lookup (returns `""` if unmatched).
- **`isNamedColor(name)`** & **`colorAt(index)`** — Fast validation and indexed table traversal.

## Usage

```cp
import {
    CSS_COLOR_COUNT,
    RgbColor,
    lookupColor,
    lookupColorHex,
    reverseLookupColor,
    isNamedColor,
} from "@cap/color-name";
```

## License

MIT
