# cobs@1.0.0 (Agent Quickref)

> Zero-allocation Consistent Overhead Byte Stuffing (COBS), custom-sentinel COBS/XOR, framed codec, and streaming CobsDecoder for CP | License: MIT
> Import: `import { ... } from "cobs";` | Full types: `src/index.d.cp`

## Functions
- `export fn max_encoded_len(source_len: usize): usize;`
- `export fn encode(src: &[]u8, dst: []u8): usize;`
- `export fn decode(src: &[]u8, dst: []u8): i64;`
- `export fn encode_with_sentinel(src: &[]u8, dst: []u8, sentinel: u8): usize;`
- `export fn decode_with_sentinel(src: &[]u8, dst: []u8, sentinel: u8): i64;`
- `export fn encode_framed(src: &[]u8, dst: []u8): usize;`
- `export fn decode_framed(src: &[]u8, dst: []u8): i64;`
