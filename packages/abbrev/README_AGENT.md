# abbrev@1.0.0 (Agent Quickref)

> Zero-heap unique unambiguous prefix abbreviation generator and resolver for CP (clean-room ISC implementation of npm abbrev). | License: ISC
> Import: `import { ... } from "abbrev";` | Full types: `src/index.d.cp`

## Functions
- `export fn abvStrEq(a: string, b: string): bool;`
- `export fn abvStrLt(a: string, b: string): bool;`
- `export fn abvHasPrefix(word: string, prefix: string): bool;`
- `export fn abvLcp(a: string, b: string): usize;`
- `export fn abbrev(words: &[]string, out: []AbbrevEntry): usize;`
- `export fn shortestAbbrev(words: &[]string, target: string): string?;`
- `export fn resolveAbbrev(words: &[]string, prefix: string): string?;`
