[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `cli-truncate`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cli-truncate` ([https://github.com/sindresorhus/cli-truncate](https://github.com/sindresorhus/cli-truncate))

Zero-heap-allocation ANSI-aware CLI string truncator in pure CP, modeled after npm `cli-truncate` v4.

Reuses [`string-width`](../string-width) for visual terminal column width measurement (ANSI/CJK/Emoji aware) and [`slice-ansi`](../slice-ansi) for ANSI SGR-preserving column slicing and automatic style resets.

## Features

- **`TruncatePosition`** — `END` (`0`), `START` (`1`), `MIDDLE` (`2`).
- **`TruncateOptions`** — Configures `position`, `prefer_truncation_on_space`, and custom `ellipsis` (default `"..."`).
- **`cliTruncateInto(text, columns, opts, out)`** — Truncates `text` to fit within `columns` terminal columns directly into caller-provided `out: []u8`:
  - Returns `text` unchanged when `stringWidth(text) <= columns`.
  - Returns `0` when `columns == 0`.
  - Preserves active ANSI SGR styles and cleanly resets them around the ellipsis.
  - Supports CJK double-width characters without splitting wide glyphs.
  - Supports `prefer_truncation_on_space` to break cleanly on space boundaries.
- **`cliTruncateEndInto` / `cliTruncateStartInto` / `cliTruncateMiddleInto`** — Ergonomic shorthand helpers with default `"..."` ellipsis.

## Usage

```cp
import {
    TruncatePosition,
    TruncateOptions,
    cliTruncateInto,
    cliTruncateEndInto,
    cliTruncateStartInto,
    cliTruncateMiddleInto,
} from "cli-truncate";

let out: [128]u8 = undefined;
const n: usize = cliTruncateEndInto("unicorn rainbow", 10, &out);
// out[0..n] == "unicorn..."
```

## License

MIT
