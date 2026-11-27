# reading-time@1.0.0 (Agent Quickref)

> Zero-allocation bilingual EN/CJK document statistics, reading-time estimator, and LLM token estimator for CP | License: MIT
> Import: `import { ... } from "reading-time";` | Full types: `src/index.d.cp`

## Functions
- `export fn decodeUtf8(s: string, pos: usize): DecodedChar;`
- `export fn isCjkChar(cp: u32): bool;`
- `export fn isWordCoreChar(cp: u32): bool;`
- `export fn isDigitByte(c: u8): bool;`
- `export fn isWsCp(cp: u32): bool;`
- `export fn isSentenceEnd(cp: u32): bool;`
- `export fn analyzeTextWithWpm(text: string, en_wpm: usize, cjk_cpm: usize): TextStats;`
- `export fn analyzeText(text: string): TextStats;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn appendUsize(out: []u8, pos: usize, val: usize): usize;`
- `export fn formatReadingTime(stats: TextStats, zh: bool, out: []u8): string;`
