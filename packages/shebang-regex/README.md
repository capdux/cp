[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/shebang-regex

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shebang-regex` ([https://github.com/sindresorhus/shebang-regex](https://github.com/sindresorhus/shebang-regex))

Zero-heap-allocation shebang (`#!`) line detector, matcher, and body-offset stripper in pure CP (parity with npm `shebang-regex`).

## Features

- `ShebangMatch { matched, line_len, cmd_start, cmd_len }`
- `hasShebang(input: string): bool`
- `matchShebang(input: string): ShebangMatch`: extracts the full `#!...` line length and post-`#!` command slice offsets, cleanly handling LF, CRLF, and EOF.
- `stripShebang(input: string): usize`: returns the byte offset of the script body right after the shebang line and its line ending.

## License

MIT
