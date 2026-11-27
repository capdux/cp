# jsonpath-rust@0.7.1 (Agent Quickref)

> Zero-heap RFC 9535 JSONPath expression parser and canonical serializer in pure CP | License: MIT
> Import: `import { ... } from "jsonpath-rust";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum JpSegKind`

## Functions
- `export fn jprIsWs(c: u8): bool;`
- `export fn jprIsDigit(c: u8): bool;`
- `export fn jprIsIdentStart(c: u8): bool;`
- `export fn jprIsIdentCont(c: u8): bool;`
- `export fn jprIsSimpleIdent(s: string): bool;`
- `export fn jprMakeSeg(kind: JpSegKind, name: string): JpSegment;`
- `export fn jprParseInt(s: string, start_pos: usize): JprIntRes;`
- `export fn parseJsonPath(expr: string, out: []JpSegment): i64;`
- `export fn jprPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn jprPushSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn jprPushI64(out: []u8, pos: usize, val: i64): usize;`
- `export fn formatJsonPath(segs: &[]JpSegment, out: []u8): usize;`
