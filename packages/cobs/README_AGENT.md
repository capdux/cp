# cobs — AI Agent Quick Ref

Zero-allocation Consistent Overhead Byte Stuffing (COBS), custom-sentinel COBS/XOR, framed codec, and streaming CobsDecoder for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CobsDecoder,
    max_encoded_len,
    encode,
    decode,
    encode_with_sentinel,
    decode_with_sentinel,
    encode_framed,
    decode_framed,
} from "cobs";
```

## Core Types & Constants

```cp
class CobsDecoder { buf: [512]u8, write_len: usize, block_rem: u8, cur_code: u8, /* +3 fields */ } // methods: init(...) +3
```

## API Signatures

```cp
fn max_encoded_len(source_len: usize): usize;
fn encode(src: &[]u8, dst: []u8): usize;
fn decode(src: &[]u8, dst: []u8): i64;
fn encode_with_sentinel(src: &[]u8, dst: []u8, sentinel: u8): usize;
fn decode_with_sentinel(src: &[]u8, dst: []u8, sentinel: u8): i64;
fn encode_framed(src: &[]u8, dst: []u8): usize;
fn decode_framed(src: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { encode } from "cobs";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = encode(out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cobs"`; install via `cap add cobs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
