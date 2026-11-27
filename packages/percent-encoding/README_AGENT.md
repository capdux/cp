# percent-encoding@1.0.0 (Agent Quickref)

> Pure CP zero-allocation WHATWG URL Standard percent-encoding and decoding (Rust percent-encoding parity) | License: MIT
> Import: `import { ... } from "percent-encoding";` | Full types: `src/index.d.cp`

## Functions
- `export fn controlsSet(): AsciiSet;`
- `export fn fragmentSet(): AsciiSet;`
- `export fn querySet(): AsciiSet;`
- `export fn pathSet(): AsciiSet;`
- `export fn userinfoSet(): AsciiSet;`
- `export fn componentSet(): AsciiSet;`
- `export fn nonAlphanumericSet(): AsciiSet;`
- `export fn percentEncodeLen(input: &[]u8, set: AsciiSet): usize;`
- `export fn percentEncode(input: &[]u8, set: AsciiSet, out: []u8): usize;`
- `export fn percentDecode(input: string, out: []u8): usize;`
- `export fn percentDecodeStrict(input: string, out: []u8): i64;`
- `export fn percentDecodePlus(input: string, out: []u8): usize;`
