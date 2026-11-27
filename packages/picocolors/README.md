[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `picocolors`

> **Source Attribution**: `npm:picocolors` ([alexeyraspopov/picocolors](https://github.com/alexeyraspopov/picocolors)) — License: MIT (upstream: ISC)

Zero-allocation, ultra-fast terminal ANSI color & style formatter with nested close-code recovery, `NO_COLOR` / `FORCE_COLOR` environment detection, single-shot `PicoFormatter`, and chainable buffered builder `PicoBuf` in pure CP (built on `ansi-styles`).

## Zero-Cost Migration (TS → CP)

Keep the exact upstream package name (`picocolors`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: picocolors)
import pc from "picocolors";
console.log(pc.bold(pc.green("READY")) + " " + pc.gray("in 12ms"));
```

```cp
// CP (zero-heap, direct buffer or PicoBuf builder)
import { PicoFormatter, PicoBuf, ColorConfig } from "picocolors";

let cfg = ColorConfig.fromEnv("", "", true); // NO_COLOR="", FORCE_COLOR="", is_tty=true
let pb = PicoBuf.fromConfig(cfg);
* = pb.green("READY");
* = pb.pushPlain(" ");
* = pb.gray("in 12ms");
let line = pb.buf[0..pb.len] as string;
```

## Installation

Add `picocolors` to your `cap.json`:

```json
{
  "dependencies": {
    "picocolors": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature / Members | Description |
| :--- | :--- | :--- |
| `detectColorSupport` | `(no_color: string, force_color: string, is_tty: bool): bool` | Standard `NO_COLOR` / `FORCE_COLOR` / TTY color support check |
| `ColorConfig` | `class { is_color_supported: bool }` — `enabled()`, `disabled()`, `fromEnv(no_color, force_color, is_tty)` | Color capability configuration |
| `PicoFormatter` | `class { cfg: ColorConfig }` — `new(bool)`, `fromConfig(cfg)`, `isColorSupported()`, `style(pair, text, out)` + all 25 modifiers/colors | Writes single styled string into caller `out: []u8`, returning written byte length |
| `PicoBuf` | `class { cfg: ColorConfig, buf: [1024]u8, len: usize }` — `new(bool)`, `fromConfig(cfg)`, `clear()`, `pushPlain(text)`, `pushStyled(pair, text)` + 19 color/style pushers | Accumulates multi-segment styled lines in a 1024-byte stack buffer |
| `picoFormatPair` / `picoMatchAt` | Low-level helpers | Direct `StylePair` formatter with nested close-code replacement |

### Available Styles & Colors

- **Modifiers (`PicoFormatter`)**: `reset`, `bold`, `dim`, `italic`, `underline`, `inverse`, `hidden`, `strikethrough`
- **Foreground**: `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`, `gray`
- **Background**: `bgBlack`, `bgRed`, `bgGreen`, `bgYellow`, `bgBlue`, `bgMagenta`, `bgCyan`, `bgWhite`
- **`PicoBuf` shorthands**: `bold`, `dim`, `italic`, `underline`, `inverse`, `strikethrough`, `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`, `gray`, `bgRed`, `bgGreen`, `bgYellow`, `bgBlue` (plus `pushStyled(pair, text)` for any `ansi-styles` `StylePair`)

## Practical Examples

### 1. Nested Style Recovery with `PicoFormatter`

```cp
import { PicoFormatter } from "picocolors";

let pc = PicoFormatter.new(true);

// Inner bold segment
let inner_buf: [64]u8 = undefined;
let n_inner = pc.bold("WARN", inner_buf[0..64]);
let inner = inner_buf[0..n_inner] as string;

// Outer yellow wraps inner safely: embedded close codes are restored automatically
let out_buf: [128]u8 = undefined;
let n_outer = pc.yellow(inner, out_buf[0..128]);
let styled = out_buf[0..n_outer] as string;
```

### 2. Building Multi-Color CLI Log Lines with `PicoBuf`

```cp
import { ColorConfig, PicoBuf } from "picocolors";

let cfg = ColorConfig.fromEnv("", "1", false); // FORCE_COLOR="1" enables color even off-TTY
let pb = PicoBuf.fromConfig(cfg);

* = pb.bgGreen(" PASS ");
* = pb.pushPlain(" ");
* = pb.bold("test_parser.cp");
* = pb.pushPlain(" ");
* = pb.dim("(4ms)");

let output = pb.buf[0..pb.len] as string;
pb.clear(); // Reuse buffer for next log line
```

## Notes & Edge Cases

- **`NO_COLOR` / `FORCE_COLOR` precedence**: `detectColorSupport` honors `NO_COLOR` (non-empty disables color), `FORCE_COLOR` (`"0"` disables, any other non-empty enables), and falls back to `is_tty`.
- **Disabled mode**: When `is_color_supported` is `false`, `PicoFormatter` and `PicoBuf` write raw plain text with zero ANSI escape sequences.
- **Buffer cap**: `PicoBuf.buf` is `[1024]u8`; call `pb.clear()` between log lines.
