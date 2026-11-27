# clf-parser@1.0.0 (Agent Quickref)

> Zero-allocation Apache/Nginx Common Log Format (CLF) and Combined Log Format parser and formatter in pure CP | License: MIT
> Import: `import { ... } from "clf-parser";` | Full types: `src/index.d.cp`

## Functions
- `export fn invalidClf(): ClfRecord;`
- `export fn isClfSpace(b: u8): bool;`
- `export fn isLineEnd(b: u8): bool;`
- `export fn parseClf(line: &[]u8): ClfRecord;`
- `export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn pushU64(dst: []u8, pos: usize, val: u64): i64;`
- `export fn formatClf(host: &[]u8, user: &[]u8, time: &[]u8, method: &[]u8, path: &[]u8, proto: &[]u8, status: u16, bytes: i64, dst: []u8): i64;`
