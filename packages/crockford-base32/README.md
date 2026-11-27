# @cap/base32-crockford

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:crockford-base32` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `crockford-base32` → CP 包名 `@cap/base32-crockford`）

Zero-allocation Douglas Crockford Base32 encoder and decoder for 64-bit integers (with optional mod-37 check symbol), arbitrary byte streams, and grouped hyphen formatting in CP.

## Features

- **Zero heap allocation**: encodes and decodes directly over caller-supplied `[]u8` buffers.
- **64-bit integer codec + mod-37 check**: `encodeU64` / `decodeU64` with optional Crockford mod-37 check symbol (`0..Z`, `*`, `~`, `$`, `=`, `U`) and 64-bit overflow protection.
- **Arbitrary byte streams**: `encodeBytes` / `decodeBytes` using unpadded 5-bit MSB-first chunks.
- **Human-friendly decoding & grouping**: ignores `-`, case-insensitive, maps `I`/`L` -> `1` and `O` -> `0`; `formatGrouped` inserts `-` every `N` characters.

## Usage

```cp
import { encodeU64, decodeU64, formatGrouped } from "@cap/base32-crockford";

let buf: [16]u8 = undefined;
const n = encodeU64(1234 as u64, true, buf[0..16]);
let val: u64 = 0 as u64;
const ok = decodeU64(buf[0..n], true, &val); // val == 1234
```
