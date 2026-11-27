[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`cli-width`](https://github.com/knownasilya/cli-width)（`ISC`）（包名 `@cap/cli-width`）。

# `@cap/cli-width`

Zero-allocation terminal column width resolver in pure CP (clean-room ISC implementation inspired by npm `cli-width`).

## Features

- **`CliWidthOptions`** — Configure `default_width`, `min_width`, and `max_width`.
- **`parseColumnsEnv(env_val)`** — Strict positive integer parser for `COLUMNS` environment values.
- **`resolveCliWidth(tty_cols, env_columns, opts)`** — Priority: `tty_cols > 0` > `COLUMNS` env > `default_width` (or `80`), with min/max clamping.
- **`defaultCliWidth()`** — Returns `80`.

## Usage

```cp
import { CliWidthOptions, parseColumnsEnv, resolveCliWidth, defaultCliWidth } from "@cap/cli-width";
```

## License

ISC
