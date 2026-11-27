# numeral@1.0.0 (Agent Quickref)

> Zero-allocation human-readable number formatting (grouped thousands, EN/ZH compact suffixes, percentages, strict Roman numerals, and English words) for CP | License: MIT
> Import: `import { ... } from "numeral";` | Full types: `src/index.d.cp`

## Functions
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn absU64(n: i64): u64;`
- `export fn formatGrouped(n: i64, sep: u8, out: []u8): string;`
- `export fn writeCompactScaled(abs: u64, unit: u64, suffix: string, next_threshold: u64, next_suffix: string, negative: bool, out: []u8): string;`
- `export fn formatCompactEn(n: i64, out: []u8): string;`
- `export fn formatCompactZh(n: i64, out: []u8): string;`
- `export fn formatPercent(ratio: f64, decimals: u8, out: []u8): string;`
- `export fn toRoman(n: u16, out: []u8): string;`
- `export fn romanCharVal(c: u8): u16;`
- `export fn fromRoman(s: string): u16?;`
- `export fn onesWord(n: u64): string;`
- `export fn tensWord(n: u64): string;`
- `export fn appendUnder1000(v: u64, out: []u8, start_pos: usize): usize;`
- `export fn appendScaleGroup(chunk: u64, label: string, out: []u8, start_pos: usize, has_prev: bool): usize;`
- `export fn toEnglishWords(n: i64, out: []u8): string;`
