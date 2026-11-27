# jest-docblock@29.7.0 (Agent Quickref)

> Zero-heap top-of-file docblock pragma extractor, parser, and formatter | License: MIT
> Import: `import { ... } from "jest-docblock";` | Full types: `src/index.d.cp`

## Functions
- `export fn jdblIsWs(c: u8): bool;`
- `export fn jdblIsLineWs(c: u8): bool;`
- `export fn jdblTrimLine(s: string): string;`
- `export fn jdblWriteStr(src: string, out: []u8, pos: usize): usize;`
- `export fn extractDocblock(code: string): string;`
- `export fn parseDocblock(docblock: string, out: []DocPragma): usize;`
- `export fn printDocblock(pragmas: &[]DocPragma, out: []u8): usize;`
