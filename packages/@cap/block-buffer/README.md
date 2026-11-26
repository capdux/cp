# @cap/block-buffer

Zero-allocation fixed-size cryptographic block streaming buffers (`BlockBuffer64`, `BlockBuffer128`) and standard padding primitives for CP (RustCrypto `block-buffer` parity).

## Features

- **Zero heap allocation**: direct pass-through for aligned full blocks and fixed internal tail buffering.
- **64-byte & 128-byte buffers**: covers SHA-256 / SHA-1 / MD5 / BLAKE2s (`BlockBuffer64`) and SHA-512 / BLAKE2b (`BlockBuffer128`).
- **Standard paddings**: Merkle-Damgård big-endian (`padMdBe64`), Merkle-Damgård little-endian (`padMdLe64`), PKCS#7 (`padPkcs7`), ISO/IEC 7816-4 (`padIso7816`), and Zero (`padZero`).

## Usage

```cp
import { BlockBuffer64 } from "@cap/block-buffer";

let bb = BlockBuffer64.new();
let blocks: [128]u8 = undefined;
const n1 = bb.feed("abc", blocks[0..128]);
const n2 = bb.padMdBe64(blocks[0..128]); // 1 block with 0x80 + 24-bit BE length
```
