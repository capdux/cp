[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/unicode-script`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-script` ([https://github.com/unicode-rs/unicode-script](https://github.com/unicode-rs/unicode-script))（原库名 `unicode-script` → CP 包名 `@cap/unicode-script`）

Zero-heap UAX #24 Unicode Script classifier in pure CP, modeled after Rust's `unicode-script` crate. Classifies Unicode scalar values across major world writing systems (`Latin`, `Greek`, `Cyrillic`, `Han`, `Hiragana`, `Katakana`, `Hangul`, `Arabic`, `Hebrew`, `Devanagari`, `Thai`, `Common`, `Inherited`, `Unknown`) and provides UTF-8 dominant script detection.

零堆分配纯 CP UAX #24 Unicode 文字系统（Script）分类与主导脚本检测库，完整对标 `crates.io:unicode-script`：支持拉丁、希腊、西里尔、汉字、平假名、片假名、韩文、阿拉伯、希伯来、天城文、泰文及 `Common`/`Inherited` 分类，内置 UTF-8 主导文字系统统计。

## Installation

```bash
cap add @cap/unicode-script
```

## API

| Symbol | Description |
| --- | --- |
| `Script` | Enum of 14 UAX #24 script variants |
| `scriptOf(cp: u32): Script` | Classify Unicode scalar value into its script |
| `scriptName(s: Script): string` | Canonical UAX #24 script name |
| `isLatin(cp: u32): bool` / `isHan(cp: u32): bool` | Fast predicates for Latin and Han |
| `isCjkScript(s: Script): bool` | True for `Han`, `Hiragana`, `Katakana`, `Hangul` |
| `dominantScript(text: string): Script` | UTF-8 dominant script detection skipping `Common`/`Inherited` |

## License

MIT OR Apache-2.0
