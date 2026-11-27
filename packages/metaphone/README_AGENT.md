# metaphone@1.0.0 (Agent Quickref)

> Zero-allocation Lawrence Philips Metaphone English phonetic encoding algorithm in pure CP | License: MIT
> Import: `import { ... } from "metaphone";` | Full types: `src/index.d.cp`

## Functions
- `export fn isVowel(b: u8): bool;`
- `export fn metaphone(word: string, max_len: usize, out: []u8): usize;`
- `export fn soundsLike(a: string, b: string): bool;`
