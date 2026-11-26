# @cap/ascii85

Zero-allocation Adobe Ascii85 (PostScript / PDF base-85) encoder and decoder for CP.

## Features

- **Zero heap allocation**: encodes and decodes directly into caller-supplied `[]u8` buffers.
- **Full Adobe Ascii85 compliance**: `!`..`u` alphabet, `'z'` all-zero 4-byte compression, optional `<~...~>` delimiters, whitespace skipping, partial tail blocks, and `u32` overflow rejection.
- **Buffer sizing helper**: `maxEncodedLen(byte_len, with_delimiters)`.

## Usage

```cp
import { encode, decode } from "@cap/ascii85";

let enc: [32]u8 = undefined;
const n = encode("Man ", true, enc[0..32]); // "<~9jqo^~>"
let decBuf: [32]u8 = undefined;
const m = decode(enc[0..n], decBuf[0..32]); // "Man "
```
