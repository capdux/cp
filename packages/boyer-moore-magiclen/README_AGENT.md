# boyer-moore-magiclen@1.0.0 (Agent Quickref)

> Zero-heap Boyer-Moore-Horspool sublinear substring search with reusable 256-byte bad-character table, reverse search, counting, and ASCII case-insensitive search. | License: MIT
> Import: `import { ... } from "boyer-moore-magiclen";` | Full types: `src/index.d.cp`

## Functions
- `export fn compilePattern(needle: &[]u8): BmhPattern;`
- `export fn findWithCompiled(haystack: &[]u8, needle: &[]u8, compiled: BmhPattern): i64;`
- `export fn findFirst(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn findLast(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn countNonOverlapping(haystack: &[]u8, needle: &[]u8): usize;`
- `export fn findIgnoreCaseAscii(haystack: string, needle: string): i64;`
