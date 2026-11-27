# @cap/nohash-hasher

Zero-overhead identity integer hasher (`IntHasher`, `nohashU64`, `nohashU32`) plus Fibonacci bucket indexing and SplitMix64 integer mixing in pure CP.

## API

- `newIntHasher(): IntHasher` (`writeU64`, `writeU32`, `writeI64`, `finish`)
- `nohashU64(key)` / `nohashU32(key)`
- `fibBucketIndex(key, bits): usize`
- `mixU64(x): u64`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`nohash-hasher@0.2.0`)
- **Upstream Repository**: https://github.com/paritytech/nohash-hasher
- **License**: Apache-2.0 (clean-room CP implementation)
