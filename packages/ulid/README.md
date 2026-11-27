[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# ulid

Pure CP, zero-allocation **ULID (Universally Unique Lexicographically Sortable Identifier)** encoder, decoder, validator, comparator, and monotonic 80-bit incrementer using Crockford's Base32.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [crates.io](https://crates.io/crates/ulid) & [npm](https://www.npmjs.com/package/ulid) |
| **Upstream Package** | `ulid` (`ulid-rs` v1 / npm `ulid` v2 / `ulidx` parity) |
| **Upstream Repo** | https://github.com/dylanhart/ulid-rs |
| **CP Package** | `ulid` (`1.0.0`) |
| **License** | MIT |

Keep the exact same package name in CP — no renaming required:

```rust
// Rust (crates.io)
use ulid::Ulid;
```

```typescript
// TypeScript (npm)
import { ulid, decodeTime } from "ulid";

// CP (cap)
import { Ulid, fromParts, timestampMs, encode, decode, isValidUlid, increment, compare } from "ulid";
```

## Installation

```bash
cap add ulid
```

## Key Features & Performance Highlights

- **128-bit Binary Layout**: 48-bit big-endian Unix millisecond timestamp + 80-bit randomness packed into `Ulid { bytes: [16]u8 }`.
- **Zero Heap Allocation**: Encodes into a caller-provided 26-byte buffer (`encode`) and decodes into `&mut Ulid` (`decode`) with zero allocations.
- **Crockford Base32 Strict & Tolerant Decoding**: Case-insensitive decoding with standard visual-ambiguity mapping (`I`/`L` -> `1`, `O` -> `0`) and 128-bit overflow rejection (first character `<= '7'`).
- **Monotonic Same-Millisecond Ordering**: `increment(u, &mut next)` adds 1 to the 80-bit random payload and returns `false` on overflow, guaranteeing strict lexicographic ordering within the same millisecond.

## Quick Start

### Construct, Encode, Decode & Compare

```typescript
import {
    Ulid,
    fromParts,
    timestampMs,
    encode,
    decode,
    isValidUlid,
    increment,
    compare,
} from "ulid";

fn main(): void {
    const entropy: [10]u8 = [0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x01, 0x02];
    const id = fromParts(1700000000000, entropy);

    // Encode to 26-character Crockford Base32
    let text_buf: [26]u8 = undefined;
    const written = encode(id, text_buf[0..]); // written == 26
    _ = written;

    // Monotonic increment within the same millisecond
    let next_id: Ulid = undefined;
    const inc_ok = increment(id, &mut next_id);
    if (inc_ok) {
        const ord = compare(id, next_id); // ord < 0
        _ = ord;
    }

    // Validate & decode string
    const sample = "01ARZ3NDEKTSV4RRFFQ69G5FAV";
    if (isValidUlid(sample)) {
        let parsed: Ulid = undefined;
        if (decode(sample, &mut parsed)) {
            const ts = timestampMs(parsed); // 1469918176385
            _ = ts;
        }
    }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Ulid` | `class { bytes: [16]u8 }` | 128-bit binary ULID (48-bit ms timestamp + 80-bit entropy) |
| `fromParts` | `(timestamp_ms: u64, rand_bytes: [10]u8) -> Ulid` | Construct ULID from Unix ms timestamp and 10 random bytes |
| `timestampMs` | `(u: Ulid) -> u64` | Extract 48-bit Unix millisecond timestamp |
| `encode` | `(u: Ulid, out: []u8) -> usize` | Encode ULID into 26-byte Crockford Base32 (returns `26` or `0` if `out.len < 26`) |
| `decode` | `(s: string, out: &mut Ulid) -> bool` | Decode 26-char Crockford Base32 string into `Ulid` |
| `isValidUlid` | `(s: string) -> bool` | Validate 26-char length, alphabet, and 128-bit overflow bound |
| `increment` | `(u: Ulid, out: &mut Ulid) -> bool` | Monotonically increment 80-bit entropy (`false` on 80-bit overflow) |
| `compare` | `(a: Ulid, b: Ulid) -> i32` | Lexicographic byte comparison (`-1`, `0`, `1`) |

## License

MIT
