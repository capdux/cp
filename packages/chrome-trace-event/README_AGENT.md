# chrome-trace-event@1.0.4 (Agent Quickref)

> Zero-heap Chrome DevTools / Perfetto Trace Event JSON serializer | License: MIT
> Import: `import { ... } from "chrome-trace-event";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum TracePhase`

## Functions
- `export fn phaseCode(ph: TracePhase): u8;`
- `export fn ctevHexDigit(n: u8): u8;`
- `export fn ctevWriteByte(out: []u8, pos: i64, b: u8): i64;`
- `export fn ctevWriteStr(out: []u8, pos: i64, s: string): i64;`
- `export fn ctevWriteJsonStr(out: []u8, pos: i64, s: string): i64;`
- `export fn ctevWriteU64(out: []u8, pos: i64, val: u64): i64;`
- `export fn formatTraceEvent(ev: TraceEvent, out: []u8): usize;`
- `export fn formatTraceArray(events: &[]TraceEvent, out: []u8): usize;`
