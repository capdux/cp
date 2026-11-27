# regex-escape@1.0.0 (Agent Quickref)

> Zero-heap regular expression literal escaper, character-class escaper, unescaper, and metacharacter detector. | License: MIT
> Import: `import { ... } from "regex-escape";` | Full types: `src/index.d.cp`

## Functions
- `export fn isMetaChar(c: u8): bool;`
- `export fn isEscapeTarget(c: u8): bool;`
- `export fn needsEscape(input: string): bool;`
- `export fn escapedLen(input: string): usize;`
- `export fn escapeRegex(input: string, out: []u8): usize;`
- `export fn escapeCharClass(input: string, out: []u8): usize;`
- `export fn unescapeRegexLiteral(input: string, out: []u8): usize;`
