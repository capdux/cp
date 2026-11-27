[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/vte

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:vte` ([https://github.com/alacritty/vte](https://github.com/alacritty/vte))

Zero-heap-allocation ANSI/VT100 terminal escape sequence state machine parser based on the Paul Williams VT500 model in pure CP.

## Features

- `VteActionKind` enum: `Print`, `Execute`, `CsiDispatch`, `OscDispatch`, `EscDispatch`
- `VteEvent { kind, ch, final_byte, param_count, params: [8]u16, intermediate }`
- `VteParser` streaming state machine (`Ground`, `Escape`, `CsiEntry`, `CsiParam`, `OscString`, `CsiIgnore`, `OscEsc`)
- Supports chunked streaming input across arbitrary byte boundaries, C0 control characters (`\r`, `\n`, `\t`, BEL), CSI sequences (SGR `\x1b[31;1m`, CUP `\x1b[10;20H`, DEC private modes `\x1b[?25h`), ESC sequences, and OSC sequences (BEL `\x07` and ST `\x1b\\` terminators)

## License

Apache-2.0
