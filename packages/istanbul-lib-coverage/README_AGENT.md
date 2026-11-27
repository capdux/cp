# istanbul-lib-coverage@3.2.2 (Agent Quickref)

> Zero-heap code coverage metrics, summary aggregation, and LCOV codec | License: BSD-3-Clause
> Import: `import { ... } from "istanbul-lib-coverage";` | Full types: `src/index.d.cp`

## Functions
- `export fn icovWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn icovWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn icovWriteU32(out: []u8, pos: usize, val: u32): usize;`
- `export fn icovTrim(s: string): string;`
- `export fn icovHasPrefix(s: string, prefix: string): bool;`
- `export fn icovStrEq(a: string, b: string): bool;`
- `export fn icovParseU32(s: string): u32;`
- `export fn icovIndexOfByte(s: string, target: u8): usize;`
- `export fn formatLcovRecord(file_path: string, hits: &[]LineHit, out: []u8): usize;`
- `export fn parseLcovSummary(lcov_text: string): CoverageMetric;`
