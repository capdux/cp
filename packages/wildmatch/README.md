[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/wildmatch

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wildmatch` ([https://github.com/becheran/wildmatch](https://github.com/becheran/wildmatch))

Clean-room MIT, zero-heap iterative wildcard pattern matcher for CP — modeled on Rust's `wildmatch` crate.

## Dependencies
- `@cap/unicase` (`1.0.0`) — reused for ASCII case-insensitive matching.

## API
- `WildMatch` (`new`, `newWithWildcards`, `newIgnoreCase`, `newWithEscape`, `matches`)
- `wildmatch(pattern, input)` — standard `*` / `?` matching
- `wildmatchIgnoreCase(pattern, input)` — case-insensitive matching
- `wildmatchCustom(pattern, input, any_many, any_one)` — custom wildcards (e.g., SQL `LIKE` `%` and `_`)
- `hasWildcards(pattern)` — quick check for wildcard characters
