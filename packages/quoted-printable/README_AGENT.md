# quoted-printable@1.0.0 (Agent Quickref)

> Zero-allocation RFC 2045 Section 6.7 MIME Quoted-Printable encoder and decoder with 76-col soft line breaks in pure CP | License: MIT
> Import: `import { ... } from "quoted-printable";` | Full types: `src/index.d.cp`

## Functions
- `export fn hexUpper(nibble: u8): u8;`
- `export fn hexVal(b: u8): i32;`
- `export fn isDirectPrintable(b: u8): bool;`
- `export fn encode(data: &[]u8, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`
