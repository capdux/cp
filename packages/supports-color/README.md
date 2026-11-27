[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `supports-color`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:supports-color` ([https://github.com/chalk/supports-color](https://github.com/chalk/supports-color))

Zero-heap-allocation terminal color capability detection in pure CP (clean-room MIT implementation inspired by npm `supports-color` v9).

Reuses [`has-flag`](../has-flag) for CLI flag detection (`--no-color`, `--color=16m`, `--color=256`, `--color`, etc., honoring `--`).

## Precedence Rules

1. **CLI Flags** (`--no-color`/`--color=false`/`--color=never` → 0, `--color=16m`/`--color=truecolor` → 3, `--color=256` → 2, `--color`/`--color=always` → 1)
2. **`FORCE_COLOR`** (`0`/`false` → 0, `1`/`true` → 1, `2` → 2, `3` → 3)
3. **`NO_COLOR`** (non-empty disables color → 0)
4. **Non-TTY** → 0
5. **`TERM=dumb`** → 0
6. **`COLORTERM=truecolor` / `24bit`** → 3
7. **Windows 10 build** (`>= 14931` → 3, `>= 10586` → 2), **CI** (GitHub Actions / GitLab / CircleCI / Travis), **`TERM_PROGRAM`** (`iTerm.app` / `Apple_Terminal`), and **`TERM`** (`-256color`, `xterm`, `screen`, `vt100`, `rxvt`, `ansi`, `linux`, etc.).

## Usage

```cp
import {
    TerminalEnv,
    ColorSupport,
    detectColorLevel,
    detectColorSupport,
    translateLevel,
} from "supports-color";
```

## License

MIT
