# words-count@1.0.0 (Agent Quickref)

> Zero-allocation multilingual (Latin + CJK) word/character/line counter and reading-time estimator for CP. | License: MIT
> Import: `import { ... } from "words-count";` | Full types: `src/index.d.cp`

## Functions
- `export fn isCjkCodepoint(cp: u32): bool;`
- `export fn analyze(text: string): TextCounts;`
- `export fn countWords(text: string): usize;`
- `export fn estimateReadingSeconds(text: string, words_per_minute: u32): u32;`
