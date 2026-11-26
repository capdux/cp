# @cap/ulid-crate

Zero-allocation 128-bit lexicographically sortable ULID encoder, decoder, and monotonic generator for CP — clean-room MIT parity with crates.io `ulid` v1 and the ULID Specification.

## Installation

```json
{
  "dependencies": {
    "@cap/ulid-crate": "1.0.0"
  }
}
```

## Quick Start

```cp
import { Ulid, encodeUlid, decodeUlid, isValidUlid } from "@cap/ulid-crate";

const id = Ulid.fromParts(1469918176385, 0x1234, 0x0123456789ABCDEF);
let buf: [26]u8 = undefined;
const n = encodeUlid(id, buf[0..]); // 26-char Crockford Base32 string

const next = id.increment(); // Monotonic +1 on 80-bit randomness
const parsed = decodeUlid(buf[0..n]);
```

## API Reference

### `Ulid { hi: u64, lo: u64 }`
- `Ulid.fromParts(timestamp_ms: u64, rand_hi16: u16, rand_lo64: u64): Ulid`
- `Ulid.fromBytes(src: []const u8): Ulid?` — 16-byte big-endian decode
- `timestampMs(): u64` — extracts the 48-bit Unix millisecond timestamp
- `cmp(other: Ulid): i32` — unsigned 128-bit / lexicographic order (`-1`, `0`, `1`)
- `eq(other: Ulid): bool`
- `increment(): Ulid?` — monotonic +1 on the 80-bit random portion; returns `null` on overflow
- `toBytes(out: []u8): usize` — writes 16 big-endian bytes

### Functions
- `encodeUlid(u: Ulid, out: []u8): usize` — 26-char uppercase Crockford Base32 (`0123456789ABCDEFGHJKMNPQRSTVWXYZ`)
- `decodeUlid(s: string): Ulid?` — strict 26-char decoder with case-insensitivity, Crockford aliases (`I`/`L` -> `1`, `O` -> `0`), and 128-bit overflow rejection (first char > `'7'`)
- `isValidUlid(s: string): bool`

## License

MIT
