[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# soundex-code

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:soundex-code` ([https://github.com/words/soundex-code](https://github.com/words/soundex-code)) / `crates.io:rphonetic` ([https://github.com/dalboris/rphonetic](https://github.com/dalboris/rphonetic))（原库名 `soundex-code` → CP 包名 `soundex-code`）

Zero-heap NARA American Soundex phonetic algorithm encoder and fuzzy name matcher for CP.
Compatible with npm `soundex-code` and Rust `rphonetic`.

## API

- `soundex(word: string, out: []u8): usize`
- `soundsLike(a: string, b: string): bool`
- `soundexDifference(a: string, b: string): u8`

## License

MIT
