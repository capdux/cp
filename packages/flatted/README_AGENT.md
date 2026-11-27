# flatted@1.0.0 (Agent Quickref)

> Zero-allocation circular-reference-safe flattened JSON serializer and parser with fixed 64-slot arena graph (clean-room implementation of npm flatted, #208 / 210M wk) | License: ISC
> Import: `import { ... } from "flatted";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum FlatNodeKind`

## Functions
- `export fn fltEmptyNode(): FlatNode;`
- `export fn fltEmptyEdge(): FlatEdge;`
- `export fn fltStrEq(a: string, b: string): bool;`
- `export fn fltPush(out: []u8, pos: usize, b: u8): usize;`
- `export fn fltPushSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn fltPushUsize(out: []u8, pos: usize, val: usize): usize;`
- `export fn fltPushI64(out: []u8, pos: usize, val: i64): usize;`
- `export fn fltPushJsonString(out: []u8, pos: usize, s: string): usize;`
- `export fn fltNeedsSlot(kind: FlatNodeKind): bool;`
- `export fn stringifyFlatted(arena: FlatArena, root_id: usize, out: []u8): usize;`
- `export fn fltSkipWs(s: string, pos: usize): usize;`
- `export fn fltParseStringSpan(s: string, pos: usize): FltSpan;`
- `export fn fltSkipValue(s: string, pos: usize): i64;`
- `export fn fltParseUsizeStr(s: string): i64;`
- `export fn fltParseI64Token(s: string): i64?;`
- `export fn fltParseInlineValue(json: string, start: usize, end: usize, slot_to_node: []usize, slot_count: usize, arena: &mut FlatArena): i64;`
- `export fn parseFlatted(json: string, out_arena: &mut FlatArena): i64;`
