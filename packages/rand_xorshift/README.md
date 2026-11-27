# @cap/xorshift

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_xorshift` ([https://github.com/rust-random/rngs](https://github.com/rust-random/rngs)) / `npm:xorshift` ([https://github.com/AndreasMadsen/xorshift](https://github.com/AndreasMadsen/xorshift))（原库名 `rand_xorshift` → CP 包名 `@cap/xorshift`）

Pure CP, zero-allocation Marsaglia Xorshift PRNG family (parity with Rust `rand_xorshift`).

## Generators

- **`XorShift128`** (exact `rand_xorshift::XorShiftRng` 128-bit state transition):
  - `fromSeed(seed: [16]u8)` (safe non-zero fallback on all-zero seed)
  - `fromU64(seed: u64)` (SplitMix64 seeding)
  - `nextU32(*_)`, `nextU64(*_)`, `nextF64(*_)` (`[0, 1)` 53-bit precision), `fillBytes(*_, out: []u8)`
- **`XorShift64Star`** (64-bit xorshift* with overflow-safe wrapping multiplication):
  - `new(seed: u64)`, `nextU64(*_)`, `nextU32(*_)`
- **`XorShift32`** (compact 32-bit xorshift):
  - `new(seed: u32)`, `nextU32(*_)`

## License

MIT
