# @cap/punycode

Zero-allocation RFC 3492 Punycode / IDNA (`xn--...`) encoder and decoder for CP.

## Features

- **Zero heap allocation**: pure CP RFC 3492 Bootstring implementation operating on caller-supplied `[]u8` buffers.
- **Label codec**: `encodeLabel` / `decodeLabel` with strict UTF-8 validation and integer overflow protection.
- **Full domain conversion**: `toAsciiDomain` / `toUnicodeDomain` handling dot-separated labels (`bücher.example.com` <-> `xn--bcher-kva.example.com`).

## Usage

```cp
import { toAsciiDomain, toUnicodeDomain } from "@cap/punycode";

let ascii: [128]u8 = undefined;
const n = toAsciiDomain("bücher.example.com", ascii[0..128]);
// "xn--bcher-kva.example.com"
```
