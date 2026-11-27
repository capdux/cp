# nibble_vec (AI Agent Quick Ref)

Zero-heap 4-bit nibble vector (`NibbleVec`, max 128 nibbles / 64 bytes inline) for radix tries and prefix compression.

## Import
```typescript
import { NibbleVec, nvecMaxNibbles, nvecGetRaw } from "nibble_vec";
```

## API Signatures
- `nvecMaxNibbles(): usize` (`128`, backed by `[64]u8`)
- `nvecGetRaw(data: [64]u8, idx: usize): u8`
- `class NibbleVec { data: [64]u8, nibble_len: usize }`
  - Constructors:
    - `NibbleVec.new(): NibbleVec`
    - `NibbleVec.fromBytes(bytes: &[]u8): NibbleVec`
    - `NibbleVec.fromByteSlice(bytes: &[]u8): NibbleVec`
  - Read-only queries & value transforms:
    - `len(): usize`, `isEmpty(): bool`, `byteLen(): usize`
    - `copyBytes(dst: []u8): usize`, `get(idx: usize): u8?`
    - `join(other: NibbleVec): NibbleVec`, `slice(start: usize, end: usize): NibbleVec`
    - `commonPrefixLen(other: NibbleVec): usize`, `hasPrefix(prefix: NibbleVec): bool`, `equals(other: NibbleVec): bool`
  - Mutable receiver (`*self`):
    - `asBytes(): &[]u8`
    - `push(nib: u8): bool`, `pop(): u8?`, `set(idx: usize, nib: u8): bool`
    - `clear()`, `truncate(new_len: usize)`
    - `split(idx: usize): NibbleVec`, `extend(other: NibbleVec): bool`

## Minimal Example
```typescript
import { NibbleVec } from "nibble_vec";

fn splitCommon(a_raw: &[]u8, b_raw: &[]u8): NibbleVec {
    let a = NibbleVec.fromBytes(a_raw);
    let b = NibbleVec.fromBytes(b_raw);
    let lcp = a.commonPrefixLen(b);
    return a.split(lcp);
}
```

## Pitfalls & Borrowing Rules
- Hard capacity limit is `128` nibbles (`64` bytes); `push`/`extend` return `false` when full, and `fromBytes` silently clamps inputs >64 bytes.
- `asBytes()` requires `*self` in `src/index.cp` so the returned `&[]u8` borrows `self.data`; bind with `let nv = ...` before calling `asBytes()`, `push`, `pop`, `set`, `clear`, `truncate`, `split`, or `extend`.
- `get(idx)` and `pop()` return optional `u8?`; unwrap with `if (let v = nv.get(i))`.
- High nibble is stored at even indices (`0, 2, ...`), low nibble at odd indices (`1, 3, ...`); `push`/`set` mask `nib & 0x0F`.
- `split(idx)` mutates `self` to keep `[0..idx)` and returns the suffix `[idx..len)` by value.
