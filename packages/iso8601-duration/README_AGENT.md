# iso8601-duration@1.0.0 (Agent Quickref)

> Zero-allocation ISO 8601 duration parser, canonical formatter, and millisecond converter in pure CP | License: MIT
> Import: `import { ... } from "iso8601-duration";` | Full types: `src/index.d.cp`

## Functions
- `export fn invalidDuration(): IsoDuration;`
- `export fn parseIsoDuration(src: &[]u8): IsoDuration;`
- `export fn pushByte(dst: []u8, pos: usize, b: u8): i64;`
- `export fn pushU32(dst: []u8, pos: usize, val: u32): i64;`
- `export fn pushComp(dst: []u8, pos: usize, val: u32, unit: u8): i64;`
- `export fn formatIsoDuration(d: IsoDuration, dst: []u8): i64;`
- `export fn toApproxMillis(d: IsoDuration): i64;`
