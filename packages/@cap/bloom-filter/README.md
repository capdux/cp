# `@cap/bloom-filter`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bloom-filters` ([https://github.com/Callidon/bloom-filters](https://github.com/Callidon/bloom-filters)) / `crates.io:bloomfilter` ([https://github.com/jedisct1/rust-bloom-filter](https://github.com/jedisct1/rust-bloom-filter))（原库名 `bloom-filters` → CP 包名 `@cap/bloom-filter`）

Zero-heap classic Bloom filter in pure CP using Kirsch-Mitzenmacher double hashing (`FNV-1a 64-bit` + `DJB2/Murmur3 fmix64`).

## Features

- **100% Zero Heap Allocation**: Operates directly on caller-provided `[]u8` bitset buffers of any size.
- **Overflow-Free 64-bit Hashing**: Wrapping arithmetic (`+%`, `*%`) safe in both Debug and Release modes.
- **Kirsch-Mitzenmacher Double Hashing**: Generates `k` independent bit positions from two 64-bit hashes.
- **Set Union (`merge`) & Diagnostics**: Includes bitwise OR `merge`, `popcount`, `fillRatio`, and `optimalNumHashes`.

## Quick Start

```cp
import { BloomFilter, optimalNumHashes, popcount, fillRatio } from "@cap/bloom-filter";

let bits: [1024]u8 = undefined;
const k = optimalNumHashes(bits.len * 8, 200);
let bf = BloomFilter.new(&bits, k);

bf.insert(&bits, "alice@example.com");
bf.insert(&bits, "bob@example.com");

const hit = bf.contains(&bits, "alice@example.com"); // true
const ratio = fillRatio(&bits);
```

## License

MIT
