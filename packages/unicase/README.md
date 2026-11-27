[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# unicase

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicase` ([https://github.com/seanmonstar/unicase](https://github.com/seanmonstar/unicase))

Clean-room MIT, zero-heap Unicode & ASCII case-insensitive comparison, ordering, matching, and hashing library for CP — modeled on Rust's `unicase` crate.

## Dependencies
- `@cap/fnv-hash` (`1.0.0`) — reused for 64-bit FNV-1a case-folded hashing.

## API
- `UniCase` (`new`, `ascii`, `intoInner`, `eq`, `cmp`, `hash`) — full Unicode case-insensitive wrapper (supports ASCII, Latin-1, `ß` ↔ `SS`, Greek final sigma `ς` ↔ `σ`, Cyrillic, Kelvin/Angstrom signs, fullwidth Latin).
- `Ascii` (`new`, `intoInner`, `eq`, `cmp`, `hash`) — fast ASCII-only case-insensitive wrapper.
- `eqUnicode`, `cmpUnicode`, `hashUnicode`, `caseFold`, `foldCodepoint`
- `eqAscii`, `cmpAscii`, `hashAscii`, `foldAsciiByte`
- `startsWithIgnoreCase`, `endsWithIgnoreCase`, `containsIgnoreCase`
