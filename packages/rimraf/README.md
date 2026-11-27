[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# rimraf

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rimraf` ([https://github.com/isaacs/rimraf](https://github.com/isaacs/rimraf))（原库名 `rimraf` → CP 包名 `rimraf`）

Zero-allocation, Clean-room MIT implementation of `rimraf` v6 core safety guards and bottom-up deletion planning for CP, reusing `@cap/fast-glob`.

## Features

- **`RimrafOptions`**: `{ preserve_root, max_retries, retry_delay_ms, is_windows }` with `RimrafOptions.defaults()` (`preserve_root: true`).
- **`isRootPath(path: string, is_windows: bool): bool`**: Accurately detects dangerous root paths (`/`, `//`, `""`, and Windows drive roots `C:\`, `C:/`, `C:`).
- **`validateRimrafTarget(target: string, opts: RimrafOptions): bool`**: Rejects empty paths, NUL-byte paths, and root paths when `preserve_root` is enabled.
- **`planBottomUpDelete(entries: []string, opts: RimrafOptions): RimrafPlan`**: Filters out unsafe targets and orders paths deepest-first so files and nested directories are removed before their parents.
- **`planFilteredDelete(entries: []string, pattern: string, opts: RimrafOptions): RimrafPlan`**: Reuses `@cap/fast-glob`'s `FastGlobOptions` and `GlobPatternSet` to filter targets by glob pattern and return a deepest-first deletion plan.

## Dependencies

- `@cap/fast-glob` (`1.0.0`)

## License

MIT
