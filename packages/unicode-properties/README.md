[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/unicode-properties`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-properties` ([https://github.com/unicode-rs/unicode-properties](https://github.com/unicode-rs/unicode-properties))

Zero-allocation Unicode character properties, UAX #31 identifier validation (`XID_Start` / `XID_Continue`), `White_Space`, `Default_Ignorable_Code_Point`, and UTS #51 Emoji classification in pure CP.

## API

- `isXidStart(cp: u32): bool` — Unicode `XID_Start` (ASCII letters, Latin Extended, Greek, Cyrillic, CJK Unified Ideographs, Hiragana, Katakana, Hangul, etc.)
- `isXidContinue(cp: u32): bool` — Unicode `XID_Continue` (`XID_Start` + digits `0-9` + `_` + combining marks + connector punctuation)
- `isUnicodeWhitespace(cp: u32): bool` — Exact match for all 25 Unicode `White_Space` codepoints (`U+0009..U+000D`, `U+0020`, `U+0085`, `U+00A0`, `U+1680`, `U+2000..U+200A`, `U+2028`, `U+2029`, `U+202F`, `U+205F`, `U+3000`)
- `isEmoji(cp: u32): bool` — UTS #51 `Emoji` property
- `isEmojiPresentation(cp: u32): bool` — UTS #51 `Emoji_Presentation` property (default colorful emoji presentation)
- `isDefaultIgnorable(cp: u32): bool` — Unicode `Default_Ignorable_Code_Point` (`U+00AD`, `U+200B..U+200F`, `U+FEFF`, `U+FE00..U+FE0F`, etc.)
- `isValidIdentifier(s: string): bool` — Strict UTF-8 validation for Unicode identifiers (`_` or `XID_Start`, followed by `XID_Continue*`)

## Example

```cp
import {
    isXidStart,
    isXidContinue,
    isUnicodeWhitespace,
    isEmoji,
    isEmojiPresentation,
    isDefaultIgnorable,
    isValidIdentifier,
} from "@cap/unicode-properties";

const ok = isValidIdentifier("变量_αβ1"); // true
const ws = isUnicodeWhitespace(0x3000 as u32); // true (ideographic space)
const rocket = isEmojiPresentation(0x1F680 as u32); // true (🚀)
```
