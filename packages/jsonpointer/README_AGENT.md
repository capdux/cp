# jsonpointer@0.1.0 (Agent Quickref)

> Zero-allocation RFC 6901 JSON Pointer validator, escaper, segment navigator, array index parser, and iterator in pure CP | License: MIT
> Import: `import { ... } from "jsonpointer";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValid(ptr: string): bool;`
- `export fn escapeToken(token: string, out: []u8): usize;`
- `export fn unescapeToken(token: string, out: []u8): usize;`
- `export fn countSegments(ptr: string): usize;`
- `export fn hasSegment(ptr: string, index: usize): bool;`
- `export fn getSegment(ptr: string, index: usize, out: []u8): usize;`
- `export fn parseArrayIndex(token: string): i64;`
- `export fn appendSegment(base: string, token: string, out: []u8): usize;`
- `export fn parentPointer(ptr: string): string;`
