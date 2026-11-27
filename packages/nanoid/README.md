[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# nanoid

Pure CP, zero-heap, URL-safe, bitmask-unbiased unique ID generator with custom alphabet and deterministic seeded PRNG support.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [npm](https://www.npmjs.com/package/nanoid) |
| **Upstream Package** | `nanoid` (v5 parity) |
| **Upstream Repo** | https://github.com/ai/nanoid |
| **CP Package** | `nanoid` (`1.0.0`) |
| **License** | MIT |

Keep the exact same package name in CP — no renaming required:

```typescript
// TypeScript (npm)
import { nanoid, customAlphabet } from "nanoid";

// CP (cap)
import {
    URL_ALPHABET,
    DEFAULT_SIZE,
    nanoidFromBytes,
    customAlphabetFromBytes,
    NanoIdGenerator,
    nanoidSeeded,
    isValidNanoId,
} from "nanoid";
```

## Installation

```bash
cap add nanoid
```

## Key Features & Security Highlights

- **Zero Heap Allocation**: All IDs are written directly into caller-provided `[]u8` slices.
- **Bitmask Rejection Sampling (Zero Modulo Bias)**: `customAlphabetFromBytes` and `NanoIdGenerator.nextCustomInto` compute `nidBitmask(alphabet.len)` and reject out-of-range values rather than using `%`, ensuring uniform character distribution.
- **CSPRNG-Ready**: Pass OS CSPRNG bytes directly into `nanoidFromBytes` / `customAlphabetFromBytes` for cryptographically strong IDs.
- **Deterministic Seeded Generator**: `NanoIdGenerator` and `nanoidSeeded` provide fast, reproducible IDs for tests, fixtures, and sharded simulations.

## Quick Start

### Cryptographic Bytes -> URL-Safe ID & Custom Alphabet

```typescript
import {
    DEFAULT_SIZE,
    URL_ALPHABET,
    nanoidFromBytes,
    customAlphabetFromBytes,
    isValidNanoId,
} from "nanoid";

fn main(): void {
    // Feed entropy from a CSPRNG (e.g. cap:crypto or OS random)
    let entropy: [32]u8 = [
        0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x70, 0x81,
        0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8, 0x09,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x01,
    ];

    let id_buf: [DEFAULT_SIZE]u8 = undefined;
    const len = nanoidFromBytes(entropy[0..], DEFAULT_SIZE, id_buf[0..]);
    _ = len;

    // Custom hex alphabet (unbiased bitmask rejection)
    let hex_buf: [12]u8 = undefined;
    const hex_len = customAlphabetFromBytes("0123456789abcdef", entropy[0..], 12, hex_buf[0..]);
    _ = hex_len;

    // Validate an ID against an alphabet
    const valid = isValidNanoId("V1StGXR8_Z5jdHi6B-myT", URL_ALPHABET);
    _ = valid;
}
```

### Seeded Generator (Tests & Deterministic IDs)

```typescript
import { NanoIdGenerator, nanoidSeeded, DEFAULT_SIZE } from "nanoid";

fn generateBatch(): void {
    let gen = NanoIdGenerator.new(42);
    let buf1: [DEFAULT_SIZE]u8 = undefined;
    let buf2: [10]u8 = undefined;

    const n1 = gen.nextInto(DEFAULT_SIZE, buf1[0..]);
    const n2 = gen.nextCustomInto("ABCDEFGHJKLMNPQRSTUVWXYZ23456789", 10, buf2[0..]);

    // One-shot seeded helper
    let buf3: [16]u8 = undefined;
    const n3 = nanoidSeeded(1337, 16, buf3[0..]);
    _ = n1;
    _ = n2;
    _ = n3;
}
```

## API Overview

| Symbol | Signature / Value | Description |
| :--- | :--- | :--- |
| `URL_ALPHABET` | `"useandom-26T198340PX75pxJACKVERYMINDBUSHWOLF_GQZbfghjklqvwyzrict"` | Standard 64-char URL-safe NanoID alphabet |
| `DEFAULT_SIZE` | `21` | Default NanoID length (~126 bits of entropy) |
| `nanoidFromBytes` | `(bytes: []u8, size: usize, out: []u8) -> usize` | Map raw random bytes to `URL_ALPHABET` |
| `customAlphabetFromBytes` | `(alphabet: string, bytes: []u8, size: usize, out: []u8) -> usize` | Unbiased bitmask-rejection mapping to `alphabet` |
| `NanoIdGenerator.new` | `(seed: u64) -> NanoIdGenerator` | Create deterministic PRNG-backed generator |
| `NanoIdGenerator.nextInto` | `(size: usize, out: []u8) -> usize` | Generate next default-alphabet ID |
| `NanoIdGenerator.nextCustomInto` | `(alphabet: string, size: usize, out: []u8) -> usize` | Generate next custom-alphabet ID |
| `nanoidSeeded` | `(seed: u64, size: usize, out: []u8) -> usize` | One-shot seeded URL-safe ID |
| `isValidNanoId` | `(id: string, alphabet: string) -> bool` | Verify non-empty `id` uses only bytes in `alphabet` |

## License

MIT
