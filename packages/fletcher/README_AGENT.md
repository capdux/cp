# fletcher (AI Agent Quick Ref)

Zero-heap Fletcher-16, Fletcher-32, and Fletcher-64 checksums (one-shot + streaming).

## Import
```typescript
import {
    Fletcher16, Fletcher32, Fletcher64,
    fletcher16, fletcher16Bytes, calcFletcher16,
    fletcher32, fletcher32Bytes, calcFletcher32,
    fletcher64, fletcher64Bytes, calcFletcher64,
} from "fletcher";
```

## API Signatures
- `class Fletcher16`: `new()`, `init()`, `withInitialValues(a: u8, b: u8)`; mutating `update(string)`, `updateBytes(&[]u8)`, `reset()`; read-only `digest(): u16`, `value(): u16`
- `class Fletcher32`: `new()`, `init()`, `withInitialValues(a: u16, b: u16)`; mutating `updateWords(&[]u16)`, `updateBytes(&[]u8)`, `update(string)`, `reset()`; read-only `digest(): u32`, `value(): u32`
- `class Fletcher64`: `new()`, `init()`, `withInitialValues(a: u32, b: u32)`; mutating `updateWords(&[]u32)`, `updateBytes(&[]u8)`, `update(string)`, `reset()`; read-only `digest(): u64`, `value(): u64`
- One-shot:
  - `fletcher16(string): u16`, `fletcher16Bytes(&[]u8): u16`, `calcFletcher16(&[]u8): u16`
  - `fletcher32(string): u32`, `fletcher32Bytes(&[]u8): u32`, `calcFletcher32(&[]u16): u32`
  - `fletcher64(string): u64`, `fletcher64Bytes(&[]u8): u64`, `calcFletcher64(&[]u32): u64`

## Minimal Example
```typescript
import { Fletcher16, fletcher16 } from "fletcher";

fn checksumPair(a: string, b: string): u16 {
    let h = Fletcher16.new();
    h.update(a);
    h.update(b);
    return h.digest();
}
```

## Pitfalls & Borrowing Rules
- `update`, `updateBytes`, `updateWords`, and `reset` take a mutable receiver (`*self`); instantiate hashers with `let h = Fletcher32.new();` (not `const`).
- `digest()` and `value()` are identical aliases and do NOT reset the hasher; call `h.reset()` before reusing.
- Note parameter types: `calcFletcher32` takes `&[]u16` and `calcFletcher64` takes `&[]u32`; for raw bytes use `fletcher32Bytes(&[]u8)` / `fletcher64Bytes(&[]u8)`.
- `new()` and `init()` are aliases with `(0, 0)` initial accumulators; use `withInitialValues(a, b)` for non-zero seeds (e.g. `0xFF, 0xFF`).
- All slice inputs are read-only borrows (`&[]T`) with zero heap allocation.
