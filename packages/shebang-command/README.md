[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# shebang-command

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shebang-command` ([https://github.com/kevva/shebang-command](https://github.com/kevva/shebang-command))

Zero-heap-allocation shebang executable command and argument extractor in pure CP (parity with npm `shebang-command`), directly reusing `shebang-regex`.

## Dependencies

- `shebang-regex` (`^1.0.0`)

## Features

- `ShebangCmd { ok, bin_len, arg_len }`
- `shebangCommand(input: string, out_cmd: []u8): usize`: resolves the actual command name from `#!/usr/bin/env node`, `#!/usr/bin/env -S node --experimental`, `#!/bin/bash -e`, `#!/usr/local/bin/python3`, etc.
- `parseShebangCmd(input: string, out_bin: []u8, out_arg: []u8): ShebangCmd`: extracts both command name and optional trailing arguments.

## License

MIT
