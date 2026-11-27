[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`color-convert`](https://github.com/Qix-/color-convert)（`MIT`）（包名 `@cap/color-convert`）。

# `@cap/color-convert`

Zero-heap-allocation color space conversion library in pure CP (clean-room MIT implementation aligned with npm `color-convert` v2). Composes `@cap/color-name` directly for CSS4 keyword lookups with zero table duplication.

## Features

- **Value-Type Color Models** — `Rgb` (8-bit `0..255`), `Hsv` (`h: 0..360, s/v: 0..100`), `Hwb` (`h: 0..360, w/b: 0..100`), `Hsl` (`h: 0..360, s/l: 0..100`), `Cmyk` (`c/m/y/k: 0..100`), and `RgbColor` (re-exported from `@cap/color-name`).
- **Perceptual & Print Spaces** — `rgbToHsv` / `hsvToRgb`, `rgbToHwb` / `hwbToRgb`, `rgbToHsl` / `hslToRgb`, `rgbToCmyk` / `cmykToRgb`, `rgbToGray` / `grayToRgb`.
- **Terminal ANSI Palettes** — `rgbToAnsi256` / `ansi256ToRgb` (xterm 6x6x6 color cube `16..231` + 24-step grayscale `232..255`), `rgbToAnsi16`, `ansi256ToAnsi16`, and `ansi16ToRgb` (SGR `30..37` / `90..97`).
- **Hex Codec** — Zero-allocation `rgbToHex(r, g, b, out)` (6-char uppercase ASCII) and `hexToRgb(hex)` supporting `#RGB`, `RGB`, `#RRGGBB`, and `RRGGBB`.
- **CSS Keywords** — `keywordToRgb(name)` and `rgbToKeyword(r, g, b)` delegating directly to `@cap/color-name`.

## Usage

```cp
import {
    Rgb,
    RgbColor,
    Hsv,
    Hwb,
    Cmyk,
    rgbToHsv,
    hsvToRgb,
    rgbToHwb,
    hwbToRgb,
    rgbToCmyk,
    cmykToRgb,
    rgbToAnsi256,
    ansi256ToRgb,
    rgbToAnsi16,
    ansi256ToAnsi16,
    rgbToHex,
    hexToRgb,
    keywordToRgb,
    rgbToKeyword,
} from "@cap/color-convert";
```

## License

MIT
