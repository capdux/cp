# cssstyle@1.0.0 (Agent Quickref)

> Zero-allocation CSS inline style declaration parser, query, mutator, and serializer (npm cssstyle parity) in pure CP | License: MIT
> Import: `import { ... } from "cssstyle";` | Full types: `src/index.d.cp`

## Functions
- `export fn cstToLower(c: u8): u8;`
- `export fn cstIsSpace(c: u8): bool;`
- `export fn cstEqIgnoreCase(a: string, b: string): bool;`
- `export fn cstTrim(s: string): string;`
- `export fn cstSkipSpaceAndComments(s: string, pos: usize): usize;`
- `export fn cstStripTrailingComments(s: string): string;`
- `export fn cstExtractImportant(raw: string): CstParsedValue;`
- `export fn cstWriteBytes(out: []u8, pos: usize, src: string, lowercase: bool): usize;`
