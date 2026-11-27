# `@cap/smol-str`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:smol_str` ([https://github.com/rust-analyzer/smol_str](https://github.com/rust-analyzer/smol_str))（原库名 `smol_str` → CP 包名 `@cap/smol-str`）

Zero-heap pure-CP small string with 24-byte inline storage (SSO) and zero-copy slice fallback for strings longer than 24 bytes — clean-room CP equivalent of crates.io `smol_str`.

## Features

- `SMOL_INLINE_CAP = 24`
- `SmolStr.empty()`, `SmolStr.new(s: string)`, `SmolStr.fromInt(v: i64)`
- `len()`, `isEmpty()`, `isInline()`, `byteAt(idx: usize): u8?`
- `eq(other)`, `eqStr(s)`, `cmp(other)`, `hasPrefix(prefix)` / `smolStartsWith`, `hasSuffix(suffix)` / `smolEndsWith`
- `writeTo(out: []u8): usize`
