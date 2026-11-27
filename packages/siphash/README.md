# @cap/siphash-pure

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:siphash` ([https://github.com/jedisct1/siphash-js](https://github.com/jedisct1/siphash-js))（原库名 `siphash` → CP 包名 `@cap/siphash-pure`）

Zero-allocation SipHash-2-4 and SipHash-1-3 64-bit keyed HashDoS-resistant pseudorandom function (PRF) for CP.

## Features

- **Zero heap allocation**: pure CP with overflow-safe 64-bit modular arithmetic.
- **Standard variants**: `siphash24` / `siphash24Key16` (2-4) and `siphash13` / `siphash13Key16` (1-3, Rust `HashMap` default).
- **Verified**: validated against official Aumasson & Bernstein SipHash reference test vectors.

## Usage

```cp
import { siphash24, siphash13 } from "@cap/siphash-pure";

const h24 = siphash24(0x0706050403020100 as u64, 0x0f0e0d0c0b0a0908 as u64, "hello");
const h13 = siphash13(0x0706050403020100 as u64, 0x0f0e0d0c0b0a0908 as u64, "hello");
```
