# emoji-regex@1.0.0 (Agent Quickref)

> Zero-allocation Unicode RGI Emoji UTF-8 scanner and extractor (npm emoji-regex parity) in pure CP | License: MIT
> Import: `import { ... } from "emoji-regex";` | Full types: `src/index.d.cp`

## Functions
- `export fn isEmojiCodePoint(cp: u32): bool;`
- `export fn findNextEmoji(s: string, from_pos: usize): EmojiSpan?;`
- `export fn countEmojis(s: string): usize;`
- `export fn containsEmoji(s: string): bool;`
- `export fn stripEmojis(s: string, out: []u8): usize;`
