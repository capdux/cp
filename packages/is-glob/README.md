[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# is-glob

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-glob` ([https://github.com/micromatch/is-glob](https://github.com/micromatch/is-glob))

Zero-heap-allocation glob pattern detector in pure CP (parity with npm `is-glob`), directly reusing `@cap/is-extglob` for Bash/Micromatch extglob detection.

## Dependencies

- `@cap/is-extglob` (`^1.0.0`)

## Features

- `isGlob(str: string, strict: bool): bool`: detects `*`, `?`, leading `!`, `[...]` character classes, `{...}` brace expansions, and extglobs via `@cap/is-extglob`, honoring backslash escapes and `strict` matching rules.
- `isGlobDefault(str: string): bool`: calls `isGlob(str, true)`.
- `hasUnescapedWildcard(str: string): bool`: checks for unescaped `*` or `?`.

## License

MIT
