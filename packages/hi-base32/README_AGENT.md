# hi-base32@1.0.0 (Agent Quickref)

> Zero-allocation RFC 4648 Section 6/7 standard Base32 (A-Z2-7) and Base32hex (0-9A-V) encoder and decoder in pure CP | License: MIT
> Import: `import { ... } from "hi-base32";` | Full types: `src/index.d.cp`

## Functions
- `export fn decodeStdChar(b: u8): i32;`
- `export fn decodeHexChar(b: u8): i32;`
- `export fn encodedLen(byte_len: usize, pad: bool): usize;`
- `export fn encodeWithAlphabet(data: &[]u8, pad: bool, alphabet: string, out: []u8): usize;`
- `export fn decodeWithMode(input: string, is_hex: bool, out: []u8): i64;`
- `export fn encode(data: &[]u8, pad: bool, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`
- `export fn encodeHex(data: &[]u8, pad: bool, out: []u8): usize;`
- `export fn decodeHex(input: string, out: []u8): i64;`
