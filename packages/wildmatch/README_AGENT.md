# wildmatch@1.0.0 (Agent Quickref)

> Zero-heap linear-time wildcard (*, ?, custom wildcards & case-insensitive) pattern matcher (clean-room port of Rust wildmatch) | License: MIT
> Import: `import { ... } from "wildmatch";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn wmByteEq(a: u8, b: u8, ignore_case: bool): bool;`
- `export fn wmMatchCore(pattern: string, input: string, any_many: u8, any_one: u8, ignore_case: bool, allow_escape: bool): bool;`
- `export fn wildmatch(pattern: string, input: string): bool;`
- `export fn wildmatchIgnoreCase(pattern: string, input: string): bool;`
- `export fn wildmatchCustom(pattern: string, input: string, any_many: u8, any_one: u8): bool;`
- `export fn hasWildcards(pattern: string): bool;`
