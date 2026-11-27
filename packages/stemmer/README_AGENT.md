# stemmer@1.0.0 (Agent Quickref)

> Zero-allocation Martin Porter (1980) English Stemmer (Steps 1a-5b) in pure CP | License: MIT
> Import: `import { ... } from "stemmer";` | Full types: `src/index.d.cp`

## Functions
- `export fn toLowerAscii(b: u8): u8;`
- `export fn isConsonant(buf: &[]u8, idx: usize): bool;`
- `export fn measurePrefix(buf: &[]u8, end: usize): usize;`
- `export fn measure(word: &[]u8): usize;`
- `export fn containsVowel(buf: &[]u8, end: usize): bool;`
- `export fn endsDoubleConsonant(buf: &[]u8, end: usize): bool;`
- `export fn endsCvc(buf: &[]u8, end: usize): bool;`
- `export fn endsWith(buf: &[]u8, len: usize, suffix: string): bool;`
- `export fn replaceSuffix(buf: []u8, len: usize, suf_len: usize, rep: string): usize;`
- `export fn tryStep2(buf: []u8, len: usize): usize;`
- `export fn tryStep3(buf: []u8, len: usize): usize;`
- `export fn tryStep4(buf: []u8, len: usize): usize;`
- `export fn stem(word: string, out: []u8): usize;`
- `export fn sameStem(a: string, b: string): bool;`
