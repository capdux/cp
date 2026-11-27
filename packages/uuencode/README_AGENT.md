# uuencode@1.0.0 (Agent Quickref)

> Zero-heap POSIX/Unix uuencode and uudecode codec supporting 45-byte lines, backtick/space zero encoding, and begin/end frames. | License: MIT
> Import: `import { ... } from "uuencode";` | Full types: `src/index.d.cp`

## Functions
- `export fn uuEncChar(v: u8): u8;`
- `export fn uuDecVal(c: u8): i32;`
- `export fn bytesEq(a: string, b: string): bool;`
- `export fn startsWith(s: string, prefix: string): bool;`
- `export fn encodeLine(data: &[]u8, out: []u8): usize;`
- `export fn decodeLine(line: string, out: []u8): i64;`
- `export fn encode(data: &[]u8, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`
