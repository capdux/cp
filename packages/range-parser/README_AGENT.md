# range-parser@1.0.0 (Agent Quickref)

> Zero-heap RFC 7233 HTTP Range header parser with suffix ranges and interval combining (clean-room port of npm range-parser) | License: MIT
> Import: `import { ... } from "range-parser";` | Full types: `src/index.d.cp`

## Functions
- `export fn rpIsWs(b: u8): bool;`
- `export fn rpTrim(s: string): string;`
- `export fn rpParseU64(s: string, ok_out: []bool): u64;`
- `export fn parseRange(size: u64, header: string, combine: bool): RangeResult;`
- `export fn rangeByteLength(r: ByteRange): u64;`
