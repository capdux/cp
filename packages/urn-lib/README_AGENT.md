# urn-lib@1.0.0 (Agent Quickref)

> Zero-allocation RFC 8141 Uniform Resource Name (URN) parser, equivalence comparator, and canonical formatter in pure CP | License: MIT
> Import: `import { ... } from "urn-lib";` | Full types: `src/index.d.cp`

## Functions
- `export fn invalidUrn(): Urn;`
- `export fn isSpace(b: u8): bool;`
- `export fn isAlphaNum(b: u8): bool;`
- `export fn toLowerAscii(b: u8): u8;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn strEq(a: string, b: string): bool;`
- `export fn isValidNid(nid: string): bool;`
- `export fn parseUrn(input: string): Urn;`
- `export fn urnEquivalent(a: Urn, b: Urn): bool;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushLowerSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn formatUrn(u: Urn, out: []u8): usize;`
