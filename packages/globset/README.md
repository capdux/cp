[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/globset

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:globset` ([https://github.com/BurntSushi/ripgrep](https://github.com/BurntSushi/ripgrep))

Pure CP, zero-allocation multi-glob set matcher and override engine (clean-room MIT parity with Rust `globset` v0.4 & `ignore::overrides`).

## Design & Reuse

- **Cross-Package Composition**: Reuses `@cap/picomatch` (`PicoOptions`, `isMatchWithOptions`, `scanBase`) and `@cap/normalize-path` (`normalizePath`) with zero duplicated glob matching or path normalization logic.
- **Static-Base Prefiltering**: Each added pattern pre-scans its static directory prefix via `scanBase` to prune non-matching paths before invoking full glob matching.
- **Zero Heap Allocation**: Fixed-capacity `GlobSet` (up to 32 patterns) and `GlobOverride` (whitelist + `!` ignore precedence).

## API

### Constants & Options
- `GLOBSET_MAX_PATTERNS = 32`
- `OVERRIDE_NONE = 0`, `OVERRIDE_WHITELIST = 1`, `OVERRIDE_IGNORE = 2`
- `GlobSetOptions { case_insensitive, literal_separator, backslash_escape }` + `GlobSetOptions.defaults()`

### `GlobSet`
- `GlobSet.new()` / `GlobSet.withOptions(opts)`
- `add(*_, pattern: string): bool`
- `len(): usize`, `isEmpty(): bool`, `clear(*_)`, `patternAt(idx: usize): string`
- `isMatch(path: string): bool`
- `matches(path: string, out_indices: []usize): usize`
- `firstMatch(path: string): i32`
- `matchCount(path: string): usize`

### `GlobOverride`
- `GlobOverride.new()` / `GlobOverride.withOptions(opts)`
- `add(*_, rule: string): bool` (`pattern` to whitelist, `!pattern` to ignore)
- `len(): usize`, `isEmpty(): bool`, `clear(*_)`
- `matched(path: string): i32`
- `isAllowed(path: string): bool`

## License

MIT
