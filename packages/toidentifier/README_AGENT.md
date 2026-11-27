# toidentifier@1.0.0 (Agent Quickref)

> Convert a string of words to a PascalCase/camelCase identifier (clean-room CP implementation of npm toidentifier) | License: MIT
> Import: `import { ... } from "toidentifier";` | Full types: `src/index.d.cp`

## Functions
- `export fn tidIsAlpha(c: u8): bool;`
- `export fn tidIsDigit(c: u8): bool;`
- `export fn tidIsIdentChar(c: u8): bool;`
- `export fn tidIsSpace(c: u8): bool;`
- `export fn tidToUpper(c: u8): u8;`
- `export fn tidToLower(c: u8): u8;`
- `export fn toIdentifier(input: string, out: []u8): usize;`
- `export fn toCamelIdentifier(input: string, out: []u8): usize;`
- `export fn toConstIdentifier(input: string, out: []u8): usize;`
- `export fn isValidIdentifier(s: string): bool;`
