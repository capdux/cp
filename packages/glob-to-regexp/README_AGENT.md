# glob-to-regexp@1.0.0 (Agent Quickref)

> Zero-allocation POSIX/glob wildcard to ECMAScript/PCRE regular expression converter with extended & globstar modes in pure CP | License: MIT
> Import: `import { ... } from "glob-to-regexp";` | Full types: `src/index.d.cp`

## Functions
- `export fn pushByte(dst: []u8, pos: usize, b: u8): i64;`
- `export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn isEscapeMeta(b: u8): bool;`
- `export fn globToRegex(glob: &[]u8, opts: GlobOptions, dst: []u8): i64;`
- `export fn globToRegexDefault(glob: &[]u8, dst: []u8): i64;`
