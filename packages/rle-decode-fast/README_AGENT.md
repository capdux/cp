# rle-decode-fast@1.0.0 (Agent Quickref)

> Zero-allocation Apple/TIFF PackBits byte run-length compression and Prometheus/Gorilla-style Delta & Delta-of-Delta time-series integer encoding for CP | License: MIT
> Import: `import { ... } from "rle-decode-fast";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodePackBits(input: string, out: []u8): string?;`
- `export fn decodePackBits(encoded: string, out: []u8): string?;`
- `export fn encodeDeltaI64(values: &[]i64, out: []i64): usize;`
- `export fn decodeDeltaI64(deltas: &[]i64, out: []i64): usize;`
- `export fn encodeDeltaOfDeltaI64(values: &[]i64, out: []i64): usize;`
- `export fn decodeDeltaOfDeltaI64(dod: &[]i64, out: []i64): usize;`
