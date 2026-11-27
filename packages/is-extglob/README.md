[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/is-extglob

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-extglob` ([https://github.com/jonschlinkert/is-extglob](https://github.com/jonschlinkert/is-extglob))

Zero-heap-allocation Bash/Micromatch extended glob (`@(...)`, `!(...)`, `*(...)`, `+(...)`, `?(...)`) detector in pure CP (parity with npm `is-extglob`).

## Features

- `isExtglob(str: string): bool`: returns `true` if `str` contains an unescaped, properly closed extglob pattern.
- `findExtglobStart(str: string): i64`: returns the byte offset of the first valid extglob, or `-1` if none exists.
- `countExtglobs(str: string): usize`: counts top-level valid extglob groups (correctly handling nested parentheses and backslash escapes).

## License

MIT
