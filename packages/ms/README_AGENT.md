# ms@1.0.0 (Agent Quickref)

> Zero-heap millisecond conversion utility (clean-room CP implementation of npm ms) | License: MIT
> Import: `import { ... } from "ms";` | Full types: `src/index.d.cp`
> Deps: `itoa`

## Functions
- `export fn msUnitMultiplier(s: string, start: usize, end: usize): f64;`
- `export fn parseMs(input: string): MsParseResult;`
- `export fn msRoundDiv(ms: i64, unit: i64): i64;`
- `export fn msFormatUnit(val: i64, short_unit: string, long_singular: string, long_plural: string, long_fmt: bool, out: []u8): usize;`
- `export fn formatMs(ms: i64, long_fmt: bool, out: []u8): usize;`
