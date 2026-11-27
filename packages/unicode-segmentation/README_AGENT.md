# unicode-segmentation@1.0.0 (Agent Quickref)

> Zero-allocation UAX #29 Unicode extended grapheme cluster and word boundary segmentation for CP (clean-room MIT). | License: MIT
> Import: `import { ... } from "unicode-segmentation";` | Full types: `src/index.d.cp`

## Functions
- `export fn usegDecodeUtf8(src: string, offset: usize): UsegRune;`
- `export fn usegIsExtend(cp: u32): bool;`
- `export fn usegIsPrepend(cp: u32): bool;`
- `export fn usegIsSpacingMark(cp: u32): bool;`
- `export fn usegIsControl(cp: u32): bool;`
- `export fn usegIsExtPic(cp: u32): bool;`
- `export fn usegGraphemeProp(cp: u32): u8;`
- `export fn usegNextGraphemeEnd(src: string, start: usize): usize;`
- `export fn countGraphemes(src: string): usize;`
- `export fn nthGrapheme(src: string, idx: usize): GraphemeSpan?;`
- `export fn sliceGraphemes(src: string, start_idx: usize, max_count: usize): GraphemeSpan;`
- `export fn usegIsCjkWordChar(cp: u32): bool;`
- `export fn usegIsALetter(cp: u32): bool;`
- `export fn usegIsNumeric(cp: u32): bool;`
- `export fn usegWordProp(cp: u32): u8;`
- `export fn usegPeekWordProp(src: string, offset: usize): u8;`
- `export fn usegIsWordSpan(src: string, start: usize, end: usize): bool;`
- `export fn countUnicodeWords(src: string): usize;`
