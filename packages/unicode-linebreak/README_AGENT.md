# unicode-linebreak@1.0.0 (Agent Quickref)

> Zero-allocation pure CP UAX #14 Unicode line break opportunity detector (unicode-linebreak parity) | License: Apache-2.0
> Import: `import { ... } from "unicode-linebreak";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum BreakKind`

## Functions
- `export fn ulbrDecodeUtf8(s: string, pos: usize): UlbrDecoded;`
- `export fn ulbrClassify(cp: u32): u8;`
- `export fn ulbrEvalBreak(prev: u8, prevNonSp: u8, curr: u8): u8;`
- `export fn findLineBreaks(utf8: string, out: []LineBreak): usize;`
- `export fn countLineBreaks(utf8: string): usize;`
