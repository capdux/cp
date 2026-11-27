# fast-deep-equal@1.0.0 (Agent Quickref)

> CP package fast-deep-equal | License: MIT
> Import: `import { ... } from "fast-deep-equal";` | Full types: `src/index.d.cp`

## Functions
- `export fn isWs(ch: u8): bool;`
- `export fn skipWs(s: string, pos: usize): usize;`
- `export fn matchLit(s: string, pos: usize, lit: string): bool;`
- `export fn hexDigit(ch: u8): i32;`
- `export fn invalidSpan(): ValueSpan;`
- `export fn scanString(s: string, pos: usize): ValueSpan;`
- `export fn scanNumber(s: string, pos: usize): ValueSpan;`
- `export fn parseValueSpan(s: string, pos: usize, depth: u32): ValueSpan;`
- `export fn kindOfJson(s: string): u8;`
- `export fn isPlainObjectJson(s: string): bool;`
- `export fn parseJsonNumberF64(s: string): f64;`
- `export fn nextDecodedChar(s: string, pos: usize): DecChar;`
- `export fn jsonStringsEqual(a: string, b: string): bool;`
- `export fn countObjectMembers(obj: string): usize;`
- `export fn arraysDeepEqual(a: string, b: string, depth: u32): bool;`
- `export fn objectsDeepEqual(a: string, b: string, depth: u32): bool;`
- `export fn valuesDeepEqual(a: string, b: string, kind: u8, depth: u32): bool;`
- `export fn jsonDeepEqual(a: string, b: string): bool;`
