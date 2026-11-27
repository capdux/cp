# filenamify@1.0.0 (Agent Quickref)

> Zero-allocation cross-platform safe filename sanitizer (npm filenamify v6 parity) for CP | License: MIT
> Import: `import { ... } from "filenamify";` | Full types: `src/index.d.cp`

## Functions
- `export fn fnmIsInvalidChar(ch: u8): bool;`
- `export fn fnmHasInvalidChar(s: string): bool;`
- `export fn fnmToLowerAscii(ch: u8): u8;`
- `export fn fnmEq3IgnoreCase(s: string, a: u8, b: u8, c: u8): bool;`
- `export fn fnmMatchAt(buf: string, pos: usize, pat: string): bool;`
- `export fn fnmEndsWith(buf: string, len: usize, pat: string): bool;`
- `export fn fnmAppendStr(dst: []u8, pos: usize, src: string): usize;`
- `export fn fnmUtf8SafeLen(s: string, max_len: usize): usize;`
- `export fn isReservedFilename(name: string): bool;`
- `export fn filenamifyWithOptions(input: string, replacement: string, max_len: usize, out: []u8): usize;`
- `export fn filenamify(input: string, out: []u8): usize;`
