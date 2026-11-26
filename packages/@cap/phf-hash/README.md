# @cap/phf-hash

Zero-heap-allocation CHD (Compress, Hash, and Displace) perfect hash table builder and O(1) lookup engine in pure CP (inspired by Rust `phf_generator` / `phf_shared`).

## Features

- `hashKey(key: string, seed: u64): PhfHashes` (overflow-safe dual 64-bit mixer generating `(g, f1, f2)`)
- `displace(f1, f2, d1, d2, len): usize` (overflow-safe `((d2 + f1 * d1 + f2) % len)`)
- `buildPhf` / `buildPhfWithSeed`: searches collision-free `(d1, d2)` displacement tables over caller-provided slices
- `lookupIndex`: O(1) perfect hash lookup with exact key verification

## License

MIT
