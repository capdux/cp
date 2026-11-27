[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `cliui`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cliui` ([https://github.com/yargs/cliui](https://github.com/yargs/cliui))

Zero-heap-allocation multi-column CLI layout engine in pure CP (clean-room MIT implementation inspired by npm `cliui`).

Reuses:
- [`wrap-ansi`](../wrap-ansi) — ANSI-preserving text wrapping
- [`strip-ansi`](../strip-ansi) — ANSI escape sequence detection & stripping
- [`unicode-width`](../unicode-width) — Terminal display column width calculation

## Features

- **`Align` & `Column`** — Configure column text, fixed or auto-distributed width (`width == 0`), alignment (`Left` / `Right` / `Center`), `pad_left`, `pad_right`, and `border`.
- **`CliUi`** — Fixed-capacity zero-heap layout builder supporting `new(width)`, `div(cols)`, `span(cols)`, `resetOutput()`, and `render(out)`.
- **`formatTwoCol`** — One-call helper for standard two-column CLI help/option menus.

## Usage

```cp
import { Align, alignToU8, Column, CliUi, formatTwoCol } from "cliui";
```

## License

MIT
