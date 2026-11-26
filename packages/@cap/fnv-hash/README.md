# @cap/fnv-hash

Zero-heap FNV-1 and FNV-1a (32-bit and 64-bit) streaming non-cryptographic hash functions for CP.
Compatible with Rust `fnv` and npm `@sindresorhus/fnv1a`.

## API

- `fnv1a32(data: []const u8): u32` / `fnv1a32Update(state: u32, data: []const u8): u32`
- `fnv132(data: []const u8): u32`
- `fnv1a64(data: []const u8): u64` / `fnv1a64Update(state: u64, data: []const u8): u64`
- `fnv164(data: []const u8): u64`
- `formatHex64(h: u64, out: []u8): usize`

## License

MIT
