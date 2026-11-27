[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `slice-ansi`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:slice-ansi` ([https://github.com/chalk/slice-ansi](https://github.com/chalk/slice-ansi))

Zero-heap-allocation ANSI-preserving terminal column slicer and truncator in pure CP.

Reuses [`@cap/anstyle-parse`](../anstyle-parse) for VT100/ANSI CSI/OSC/ESC tokenization and SGR parameter parsing, and [`@cap/unicode-width`](../unicode-width) for East Asian / CJK double-width and zero-width column measurement.

## Features

- **`ansiVisibleWidth(input)`** — Computes terminal display column width while ignoring ANSI escape sequences and handling CJK double-width characters.
- **`sliceAnsi(input, begin, end, out)`** — Slices by visible display column range `[begin, end)`, re-emits active SGR styles entering the slice, skips OSC/non-SGR control codes, avoids splitting CJK wide characters, and automatically appends closing/reset codes (`\x1b[39m`, `\x1b[49m`, or `\x1b[0m`).
- **`sliceAnsiFrom(input, begin, out)`** — Slices from visible column `begin` to the end of `input`.
- **`truncateAnsi(input, max_cols, ellipsis, out)`** — Truncates to `max_cols` display columns with an ellipsis while keeping ANSI style resets clean.

## Usage

```cp
import {
    ansiVisibleWidth,
    sliceAnsi,
    sliceAnsiFrom,
    truncateAnsi,
} from "slice-ansi";
```

## License

MIT
