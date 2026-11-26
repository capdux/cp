# @cap/chacha-rng

Zero-allocation RFC 8439 ChaCha8 / ChaCha12 / ChaCha20 block function and seekable CSPRNG for CP.

## Features

- **Zero heap allocation**: fixed 64-byte block buffer with overflow-safe 32-bit modular arithmetic.
- **RFC 8439 compliant**: `chachaBlock` verified against RFC 8439 Section 2.3.2 official test vector; supports 8, 12, and 20 rounds.
- **Seekable CSPRNG**: `ChaCha20Rng` with `fromSeed`, `fromU64`, `nextU32`, `nextU64`, `nextF64`, `fillBytes`, and `setWordPos`.

## Usage

```cp
import { ChaCha20Rng } from "@cap/chacha-rng";

let rng = ChaCha20Rng.fromU64(42 as u64);
const x = rng.nextU64();
const u = rng.nextF64(); // [0.0, 1.0)
```
