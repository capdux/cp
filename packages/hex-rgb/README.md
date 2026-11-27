[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/hex-color

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hex-rgb` ([https://github.com/sindresorhus/hex-rgb](https://github.com/sindresorhus/hex-rgb)) / `npm:colord` ([https://github.com/omgovich/colord](https://github.com/omgovich/colord))（原库名 `hex-rgb` → CP 包名 `@cap/hex-color`）

Zero-allocation CSS hex color (`#RGB`, `#RGBA`, `#RRGGBB`, `#RRGGBBAA`) parser, formatter, Porter-Duff `source-over` blender, BT.601 luminance detector, and RGB inverter for CP.

## Features

- **Zero heap allocation**: pure CP parsing and formatting into caller-supplied `[]u8` buffers.
- **Full CSS hex syntax**: `parseHexColor`, `formatHex6`, `formatHex8`, `packRgba32`, `unpackRgba32`.
- **Color operations**: `invertRgb`, BT.601 `isDark`, and Porter-Duff `source-over` `blendOver`.

## Usage

```cp
import { parseHexColor, formatHex6, blendOver, isDark } from "@cap/hex-color";

const fg = parseHexColor("#ff000080");
const bg = parseHexColor("#ffffff");
const blended = blendOver(fg, bg);
let buf: [7]u8 = undefined;
const n = formatHex6(blended, false, buf[0..7]);
```
