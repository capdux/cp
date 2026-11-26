<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`emoji-regex`](https://github.com/mathiasbynens/emoji-regex)（`MIT`）（包名 `@cap/emoji-regex`）。

# `@cap/emoji-regex`

Zero-heap-allocation Unicode RGI Emoji UTF-8 scanner and extractor in pure CP (clean-room MIT implementation inspired by npm `emoji-regex`).

## Features

- **`isEmojiCodePoint(cp: u32): bool`** — Covers Unicode Emoji blocks (`U+1F300..U+1FAFF`), Misc Symbols & Dingbats (`U+2600..U+27BF`), Regional Indicators (`U+1F1E6..U+1F1FF`), Skin Tone Modifiers (`U+1F3FB..U+1F3FF`), and standalone BMP emoji.
- **`findNextEmoji(s: string, from_pos: usize): EmojiSpan?`** — Zero-allocation UTF-8 scanner supporting ZWJ (`U+200D`) sequences, Variation Selector-16 (`U+FE0F`), Skin Tone Modifiers, Keycap sequences, Regional Indicator flag pairs, and Tag sequences.
- **`countEmojis(s)` / `containsEmoji(s)` / `stripEmojis(s, out)`** — Fast counting, detection, and stripping into caller-owned buffers.

## Usage

```cp
import { EmojiSpan, isEmojiCodePoint, findNextEmoji, countEmojis, containsEmoji, stripEmojis } from "@cap/emoji-regex";
```

## License

MIT
