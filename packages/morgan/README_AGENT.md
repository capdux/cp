# morgan@1.0.0 (Agent Quickref)

> Zero-heap HTTP access log template formatter supporting Morgan tiny/dev/short/common/combined presets and custom tokens (clean-room port of morgan v1.10) | License: MIT
> Import: `import { ... } from "morgan";` | Full types: `src/index.d.cp`
> Deps: `itoa`

## Functions
- `export fn mrgIsTokenChar(c: u8): bool;`
- `export fn mrgWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn mrgWriteOrDash(out: []u8, pos: usize, s: string): usize;`
- `export fn mrgWriteResponseTime(out: []u8, pos: usize, us: u64): usize;`
- `export fn formatMorganLog(format_str: string, ev: HttpLogEvent, out: []u8): usize;`
- `export fn formatPreset(preset: string, ev: HttpLogEvent, out: []u8): usize;`
