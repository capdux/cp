# ulid — AI Agent Quick Ref

- **Package**: `ulid` (`1.0.0`) | **Source**: crates.io `ulid` & npm `ulid` (Crockford Base32 128-bit ULID)
- **Import**: `import { Ulid, fromParts, timestampMs, encode, decode, isValidUlid, increment, compare } from "ulid";`
- **Allocation**: Zero-heap (fixed 16-byte binary / 26-byte text buffers).

## Core Signatures

```typescript
export class Ulid {
    bytes: [16]u8,
}

export fn fromParts(timestamp_ms: u64, rand_bytes: [10]u8): Ulid;
export fn timestampMs(u: Ulid): u64;
export fn encode(u: Ulid, out: []u8): usize;          // writes 26 bytes, or 0 if out.len < 26
export fn decode(s: string, out: &mut Ulid): bool;
export fn isValidUlid(s: string): bool;
export fn increment(u: Ulid, out: &mut Ulid): bool;  // false on 80-bit entropy overflow
export fn compare(a: Ulid, b: Ulid): i32;            // -1, 0, or 1

// Low-level Crockford Base32 helpers
export fn ulEncodeChar(v: u8): u8;
export fn ulDecodeChar(c: u8): u8;
export fn ulEncodeBlock40(b0: u8, b1: u8, b2: u8, b3: u8, b4: u8, out: []u8, offset: usize);
```

## Minimal Pattern

```typescript
import { Ulid, fromParts, encode, decode, timestampMs } from "ulid";

const id = fromParts(1700000000000, rand10);
let out: [26]u8 = undefined;
const n = encode(id, out[0..]); // n == 26

let parsed: Ulid = undefined;
if (decode("01ARZ3NDEKTSV4RRFFQ69G5FAV", &mut parsed)) {
    const ms = timestampMs(parsed);
}
```

## Pitfalls

1. **No built-in CSPRNG/clock**: Supply `timestamp_ms` and `rand_bytes: [10]u8` explicitly to `fromParts(...)` (deterministic & testable).
2. **Output buffer length**: `encode` requires `out.len >= 26`, otherwise returns `0`.
3. **128-bit overflow check**: First character of a valid ULID must be `0`–`7`; strings starting with `8`–`Z` fail `decode`/`isValidUlid`.
4. **Mutable out-param**: `decode` and `increment` take `&mut Ulid`.
