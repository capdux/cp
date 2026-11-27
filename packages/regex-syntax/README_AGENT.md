# regex-syntax@1.0.0 (Agent Quickref)

> Zero-allocation regular expression syntax validator, metacharacter escaper, and literal prefix extractor (Rust regex-syntax v0.8 parity) | License: MIT
> Import: `import { ... } from "regex-syntax";` | Full types: `src/index.d.cp`

## Functions
- `export fn isMetaCharacter(c: u8): bool;`
- `export fn escapeRegex(input: string, out: []u8): usize;`
- `export fn isWordByte(b: u8): bool;`
- `export fn extractLiteralPrefix(pattern: string, out: []u8): usize;`
- `export fn validateRegexSyntax(pattern: string): RegexSyntaxInfo;`
