# @cap/subtle-ct

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:subtle` ([https://github.com/dalek-cryptography/subtle](https://github.com/dalek-cryptography/subtle)) / `npm:tsscmp` ([https://github.com/suryagh/tsscmp](https://github.com/suryagh/tsscmp)) / `npm:safe-compare` ([https://github.com/vadimdemedes/safe-compare](https://github.com/vadimdemedes/safe-compare))（原库名 `subtle` → CP 包名 `@cap/subtle-ct`）

Zero-heap-allocation branchless constant-time primitives and buffer zeroization in pure CP (inspired by RustCrypto `subtle` and `zeroize`), with overflow-free Debug-mode bitmask arithmetic.

## Features

- `Choice { val: u8 }`: `fromU8`, `toBool`, `not`, `and`, `or`, `xor`
- Equality: `ctEqU8`, `ctEqU32`, `ctEqU64`, `ctEqBytes`, `ctIsZeroU64`
- Ordering: `ctLtU64`, `ctGtU64` (branchless, overflow-free across full `0..u64::MAX`)
- Selection & Swap: `ctSelectU8`, `ctSelectU32`, `ctSelectU64`, `ctSelectBytes`, `ctCondSwapU64`
- Buffer wiping: `zeroizeBytes(buf: []u8)`

## License

MIT
