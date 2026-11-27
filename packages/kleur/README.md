[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `kleur`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kleur` ([https://github.com/lukeed/kleur](https://github.com/lukeed/kleur))

Zero-allocation composable chainable ANSI terminal style builder for CP with nested close and reset recovery. Clean-room MIT implementation inspired by npm `kleur` v4, composing `ansi-styles`.

## Features

- **Fluent immutable chaining**: Compose up to 8 (`KLEUR_MAX_DEPTH`) modifiers, foreground colors, and background colors with zero heap allocation (`KleurChain.new(true).bold().red().bgYellow().format(text, out)`).
- **Zero ANSI duplication**: Reuses SGR `StylePair` definitions directly from `ansi-styles`.
- **Reverse-order closing & nested recovery**: Emits open codes in stack order, closes in reverse order, and automatically restores outer styles when embedded close codes or `\x1b[0m` reset sequences appear inside the input text.
- **Disabled passthrough**: When `enabled == false` (or `depth == 0`), passes text through verbatim without ANSI escape sequences.

## API

### `KleurChain`

- Constructors: `KleurChain.new(enabled: bool)`, `KleurChain.on()`, `KleurChain.off()`
- Modifiers: `reset()`, `bold()`, `dim()`, `italic()`, `underline()`, `inverse()`, `hidden()`, `strikethrough()`
- Foregrounds: `black()`, `red()`, `green()`, `yellow()`, `blue()`, `magenta()`, `cyan()`, `white()`, `gray()`
- Backgrounds: `bgBlack()`, `bgRed()`, `bgGreen()`, `bgYellow()`, `bgBlue()`, `bgMagenta()`, `bgCyan()`, `bgWhite()`
- Execution: `format(text: string, out: []u8): usize`, `depthCount(): usize`, `withStyle(pair: StylePair): KleurChain`

## Example

```cp
import { KleurChain } from "kleur";

let out: [128]u8 = undefined;
let n = KleurChain.on().bold().red().bgYellow().format("ALERT", out[0..128]);
// out[0..n] == "\x1b[1m\x1b[31m\x1b[43mALERT\x1b[49m\x1b[39m\x1b[22m"
```

## License

MIT
