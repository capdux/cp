# nibble_vec — AI Agent Quick Ref

- **Import**: `import { NIBBLE_MAX_BYTES, nvecMaxNibbles, nvecGetRaw, NibbleVec } from "nibble_vec";`
- **Source**: crates.io `nibble_vec` (`v0.1.0`, zero-heap inline 64-byte / 128-nibble vector)

## Core Signatures
```cp
NIBBLE_MAX_BYTES: usize = 64
nvecMaxNibbles(): usize                    // 128
nvecGetRaw(data: [64]u8, idx: usize): u8

class NibbleVec {
    data: [64]u8,
    nibble_len: usize,
    static new(): NibbleVec;
    static fromBytes(bytes: &[]u8): NibbleVec;       // alias: fromByteSlice
    len(): usize;
    isEmpty(): bool;
    byteLen(): usize;                                // (nibble_len + 1) / 2
    asBytes(): &[]u8;
    copyBytes(dst: []u8): usize;
    push(nib: u8): bool;                             // false if full (>= 128)
    pop(): u8?;
    get(idx: usize): u8?;                            // null if idx >= len
    set(idx: usize, nib: u8): bool;
    clear();
    truncate(new_len: usize);
    split(idx: usize): NibbleVec;                    // mutates self to [0..idx), returns [idx..len)
    join(other: NibbleVec): NibbleVec;
    extend(other: NibbleVec): bool;                  // false if combined > 128
    slice(start: usize, end: usize): NibbleVec;
    commonPrefixLen(other: NibbleVec): usize;
    hasPrefix(prefix: NibbleVec): bool;
    equals(other: NibbleVec): bool;
}
```

## Pitfalls
- Fixed inline capacity is **128 nibbles (64 bytes)**; `fromBytes` truncates beyond 64 bytes, and `push`/`extend` return `false` when capacity is exceeded.
- Nibble values are 4-bit (`0..15`); higher bits are masked (`nib & 0x0F`).
- Even indices occupy the high nibble (`byte >> 4`), odd indices occupy the low nibble (`byte & 0x0F`).
- `split`, `push`, `pop`, `set`, `clear`, `truncate`, `extend`, and `asBytes` take a mutable receiver (`let mut nv = ...`).
