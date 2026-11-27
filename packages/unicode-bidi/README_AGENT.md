# unicode-bidi@1.0.0 (Agent Quickref)

> Zero-allocation Unicode Bidirectional Algorithm (UAX #9) classifier, base direction resolver, visual run splitter, and bracket mirroring for CP | License: MIT
> Import: `import { ... } from "unicode-bidi";` | Full types: `src/index.d.cp`

## Functions
- `export fn decodeUtf8At(text: string, pos: usize): Utf8Step;`
- `export fn bidiClass(cp: u32): u8;`
- `export fn baseDirection(text: string): u8;`
- `export fn hasRtl(text: string): bool;`
- `export fn isLtrOnly(text: string): bool;`
- `export fn resolveCharLevel(cls: u8, base_level: u8, prev_level: u8): u8;`
- `export fn computeVisualRuns(text: string, out_runs: []BidiRun): usize;`
- `export fn mirrorAsciiBracket(ch: u8): u8;`
