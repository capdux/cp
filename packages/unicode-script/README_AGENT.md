# unicode-script@1.0.0 (Agent Quickref)

> Zero-heap UAX #24 Unicode Script property classifier and dominant script detector in pure CP (modeled after Rust unicode-script) | License: MIT OR Apache-2.0
> Import: `import { ... } from "unicode-script";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum Script`

## Functions
- `export fn uscScriptIndex(s: Script): usize;`
- `export fn uscIndexToScript(idx: usize): Script;`
- `export fn scriptOf(cp: u32): Script;`
- `export fn scriptName(s: Script): string;`
- `export fn isLatin(cp: u32): bool;`
- `export fn isHan(cp: u32): bool;`
- `export fn isCjkScript(s: Script): bool;`
- `export fn uscDecodeUtf8(text: string, pos: usize): UscDecoded;`
- `export fn dominantScript(text: string): Script;`
