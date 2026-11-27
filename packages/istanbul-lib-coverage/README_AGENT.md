# istanbul-lib-coverage@3.2.2 (Agent Quickref)

> Zero-heap code coverage metrics, summary aggregation, and LCOV codec | License: BSD-3-Clause
> Import: `import { ... } from "istanbul-lib-coverage";` | Full types: `src/index.d.cp`

## Functions
- `export fn icovWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn icovParseU32(s: string): u32;`
- `export fn formatLcovRecord(file_path: string, hits: &[]LineHit, out: []u8): usize;`
- `export fn parseLcovSummary(lcov_text: string): CoverageMetric;`
