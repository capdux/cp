# path-clean@1.0.0 (Agent Quickref)

> Zero-heap lexical POSIX path cleaner following Go filepath.Clean / Rust path-clean semantics in pure CP | License: MIT OR Apache-2.0
> Import: `import { ... } from "path-clean";` | Full types: `src/index.d.cp`

## Functions
- `export fn pclByteAt(a: string, b: string, has_sep: bool, idx: usize): u8;`
- `export fn pclCleanTwo(a: string, b: string, has_sep: bool, out: []u8): usize;`
- `export fn cleanPath(path: string, out: []u8): usize;`
- `export fn cleanJoin(a: string, b: string, out: []u8): usize;`
- `export fn isClean(path: string): bool;`
