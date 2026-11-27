[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/anstyle`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:anstyle` ([https://github.com/rust-cli/anstyle](https://github.com/rust-cli/anstyle)) / `crates.io:console` ([https://github.com/console-rs/console](https://github.com/console-rs/console)) / `crates.io:colored` ([https://github.com/colored-rs/colored](https://github.com/colored-rs/colored))

Zero-heap ANSI terminal styling model in pure CP (clean-room counterpart to Rust `anstyle`).

## Features

- **100% Zero Heap Allocation**: Value-type `Color`, `Effects`, and `Style` rendering directly into caller-provided `out: []u8`.
- **Full Color Support**: Basic 16-color (`30-37`/`90-97`/`40-47`/`100-107`), 256-color (`38;5;N`/`48;5;N`), and 24-bit TrueColor RGB (`38;2;R;G;B`/`48;2;R;G;B`).
- **Complete SGR Effects**: `bold`, `dimmed`, `italic`, `underline`, `blink`, `invert`, `hidden`, `strikethrough`.
- **Clean Passthrough & Reset**: `paint(text, out)` passes plain styles through untouched and automatically appends `ESC[0m` for styled text.

## Quick Start

```cp
import { Color, Style } from "@cap/anstyle";

const base = Style.new();
const red = base.fgColor(Color.ansi(1));
const err_style = red.bold();

let buf: [64]u8 = undefined;
const n = err_style.paint("Error!", &buf); // "\x1b[1;31mError!\x1b[0m"
```

## License

MIT
