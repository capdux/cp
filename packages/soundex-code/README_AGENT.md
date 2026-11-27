# soundex-code@1.0.0 (Agent Quickref)

> Zero-heap American Soundex phonetic algorithm encoder and phonetic match engine for CP (compatible with npm soundex-code & Rust rphonetic) | License: MIT
> Import: `import { ... } from "soundex-code";` | Full types: `src/index.d.cp`

## Functions
- `export fn toUpper(c: u8): u8;`
- `export fn isAlpha(c: u8): bool;`
- `export fn soundexDigit(u: u8): u8;`
- `export fn soundex(word: string, out: []u8): usize;`
- `export fn soundsLike(a: string, b: string): bool;`
- `export fn soundexDifference(a: string, b: string): u8;`
