[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/picocolors`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:picocolors` ([https://github.com/alexeyraspopov/picocolors](https://github.com/alexeyraspopov/picocolors))

Zero-allocation ultra-fast terminal ANSI color & style formatter for CP with nested close-code recovery and `NO_COLOR`/`FORCE_COLOR` environment detection. Clean-room MIT implementation inspired by npm `picocolors` v1, composing `@cap/ansi-styles`.

## Features

- **Zero heap allocation**: Both `PicoFormatter` (writes to caller `[]u8`) and `PicoBuf` (fixed 1024-byte stack accumulator with safe truncation) allocate zero heap memory.
- **Zero ANSI duplication**: Reuses SGR `StylePair` constants directly from `@cap/ansi-styles`.
- **Nested close-code recovery**: Matches `picocolors` `replaceClose` semantics — nesting `blue` inside `red` or `dim` inside `bold` restores the outer style cleanly after the inner close code.
- **Standard color detection**: `detectColorSupport(no_color, force_color, is_tty)` follows `FORCE_COLOR`, `NO_COLOR`, and TTY precedence. When disabled, text passes through verbatim without ANSI escape sequences.

## API

### Environment Detection & Config

- `detectColorSupport(no_color: string, force_color: string, is_tty: bool): bool`
- `ColorConfig { is_color_supported: bool }` (`enabled()`, `disabled()`, `fromEnv(...)`)

### `PicoFormatter`

- `PicoFormatter.new(is_color_supported: bool)` / `PicoFormatter.fromConfig(cfg: ColorConfig)`
- Modifiers: `reset`, `bold`, `dim`, `italic`, `underline`, `inverse`, `hidden`, `strikethrough`
- Foregrounds: `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`, `gray`
- Backgrounds: `bgBlack`, `bgRed`, `bgGreen`, `bgYellow`, `bgBlue`, `bgMagenta`, `bgCyan`, `bgWhite`

### `PicoBuf`

Fixed 1024-byte (`PICO_BUF_CAP`) accumulating buffer with `clear()`, `pushPlain()`, `pushStyled()`, and convenience methods (`bold`, `dim`, `italic`, `underline`, `inverse`, `strikethrough`, `black`..`gray`, `bgRed`..`bgBlue`).

## Example

```cp
import { ColorConfig, PicoFormatter, PicoBuf } from "@cap/picocolors";

let fmt = PicoFormatter.fromConfig(ColorConfig.enabled());
let out: [128]u8 = undefined;
let n = fmt.green("OK", out[0..128]);
// out[0..n] == "\x1b[32mOK\x1b[39m"

let pb = PicoBuf.new(true);
pb.bold("Status: ");
pb.green("PASS");
// pb.buf[0..pb.len] == "\x1b[1mStatus: \x1b[22m\x1b[32mPASS\x1b[39m"
```

## License

MIT
