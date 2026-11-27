# fast-json-stable-stringify@1.0.0 (Agent Quickref)

> Zero-allocation RFC 8785 JSON Canonicalization Scheme (JCS) deterministic serializer & semantic equivalence checker (fast-json-stable-stringify / canonicalize / serde_jcs parity) for CP | License: MIT
> Import: `import { ... } from "fast-json-stable-stringify";` | Full types: `src/index.d.cp`

## Functions
- `export fn isJsonSpace(c: u8): bool;`
- `export fn skipWs(s: string, start: usize): usize;`
- `export fn parseHex4(s: string, pos: usize): i32;`
- `export fn pushByteChecked(out: []u8, pos: usize, b: u8): usize?;`
- `export fn pushSliceChecked(out: []u8, pos: usize, s: string): usize?;`
- `export fn writeUtf8Checked(out: []u8, pos: usize, cp: u32): usize?;`
- `export fn emitEscapedCodepoint(out: []u8, pos: usize, cp: u32): usize?;`
- `export fn skipString(input: string, pos: usize): SkipRes;`
- `export fn skipNumber(input: string, pos: usize): SkipRes;`
- `export fn skipValue(input: string, start: usize, depth: u8): SkipRes;`
- `export fn nextKeyUtf16Unit(input: string, pos: usize, end: usize, pending_low: u16): KeyStep;`
- `export fn compareJsonKeys(input: string, s1: usize, e1: usize, s2: usize, e2: usize): i8;`
- `export fn emitCanonicalString(input: string, pos: usize, out: []u8, out_pos: usize): StepRes;`
- `export fn emitCanonicalNumber(input: string, pos: usize, out: []u8, out_pos: usize): StepRes;`
- `export fn emitCanonicalValue(input: string, start: usize, out: []u8, out_pos: usize, depth: u8): StepRes;`
- `export fn canonicalizeJson(input: string, out: []u8): JcsResult;`
- `export fn areJsonEquivalent(a: string, b: string): bool;`
