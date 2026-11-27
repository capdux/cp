# esutils@1.0.0 (Agent Quickref)

> Zero-allocation ECMAScript keyword, reserved word, identifier, whitespace, and line terminator utilities (clean-room MIT implementation of npm esutils, #201 / 215M wk) | License: MIT
> Import: `import { ... } from "esutils";` | Full types: `src/index.d.cp`

## Functions
- `export fn isDecimalDigit(cp: u32): bool;`
- `export fn isHexDigit(cp: u32): bool;`
- `export fn isOctalDigit(cp: u32): bool;`
- `export fn isWhiteSpace(cp: u32): bool;`
- `export fn isLineTerminator(cp: u32): bool;`
- `export fn isIdentifierStartES6(cp: u32): bool;`
- `export fn isIdentifierPartES6(cp: u32): bool;`
- `export fn isKeywordES6(id: string, strict: bool): bool;`
- `export fn isReservedWordES6(id: string, strict: bool): bool;`
- `export fn isRestrictedWord(id: string): bool;`
- `export fn isIdentifierNameES6(id: string): bool;`
- `export fn isIdentifierES6(id: string, strict: bool): bool;`
