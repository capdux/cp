# `@cap/rustc-hash`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustc-hash` ([https://github.com/rust-lang/rustc-hash](https://github.com/rust-lang/rustc-hash))

Zero-heap pure-CP fast non-cryptographic `FxHasher` (64-bit & 32-bit) — clean-room CP equivalent of crates.io `rustc-hash`.

## Features

- `FX_SEED_64` (`0x517cc1b727220a95`) & `FX_SEED_32` (`0x9e3779b9` / `2654435769`)
- `FxHasher64`: `new`, `withSeed`, `writeUsize`, `writeU64`, `writeU32`, `writeBytes`, `writeStr`, `finish`, `reset`
- `FxHasher32`: `new`, `withSeed`, `writeU32`, `writeBytes`, `writeStr`, `finish`, `reset`
- One-shot helpers: `fxHash64`, `fxHash64Str`, `fxHash32`
