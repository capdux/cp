# to-regex-range@1.0.0 (Agent Quickref)

> Zero-allocation numeric range to compact regex pattern compiler and numeric range matcher for CP | License: MIT
> Import: `import { ... } from "to-regex-range";` | Full types: `src/index.d.cp`

## Functions
- `export fn bwPush(w: &mut TrrBufWriter, out: []u8, ch: u8);`
- `export fn countDigits(n: u64): usize;`
- `export fn absI64ToU64(v: i64): u64;`
- `export fn trrWriteU64(w: &mut TrrBufWriter, out: []u8, v: u64);`
- `export fn isSinglePattern(a: u64, b: u64): bool;`
- `export fn emitSubRange(w: &mut TrrBufWriter, out: []u8, a: u64, b: u64);`
- `export fn insertStop(stops: []u64, count: &mut usize, val: u64);`
- `export fn sortStops(stops: []u64, count: usize);`
- `export fn splitPositiveStops(a: u64, b: u64, stops: []u64): usize;`
- `export fn emitPositiveRange(w: &mut TrrBufWriter, out: []u8, a: u64, b: u64);`
- `export fn emitNegativeRange(w: &mut TrrBufWriter, out: []u8, mag_lo: u64, mag_hi: u64);`
- `export fn toRegexRange(min_val: i64, max_val: i64, out: []u8): usize;`
- `export fn matchNumericRange(s: string, min_val: i64, max_val: i64): bool;`
