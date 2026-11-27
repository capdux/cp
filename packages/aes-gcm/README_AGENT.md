# aes-gcm — AI Agent Quick Ref

- **Package**: `aes-gcm` (`1.0.0`) | **Source**: crates.io `aes-gcm` (NIST SP 800-38D)
- **Import**: `import { Aes128Gcm, Aes256Gcm } from "aes-gcm";`
- **Allocation**: Zero-heap (fixed-size key/nonce/tag arrays + caller output slices).

## Core Signatures

```typescript
export class Aes128Gcm {
    fn new(key: [16]u8): Aes128Gcm;
    fn gctr(nonce: [12]u8, input: &[]u8, output: []u8);
    fn encrypt(nonce: [12]u8, aad: &[]u8, plaintext: &[]u8, out_ct: []u8): [16]u8;
    fn decrypt(nonce: [12]u8, aad: &[]u8, ciphertext: &[]u8, tag: [16]u8, out_pt: []u8): bool;
}

export class Aes256Gcm {
    fn new(key: [32]u8): Aes256Gcm;
    fn gctr(nonce: [12]u8, input: &[]u8, output: []u8);
    fn encrypt(nonce: [12]u8, aad: &[]u8, plaintext: &[]u8, out_ct: []u8): [16]u8;
    fn decrypt(nonce: [12]u8, aad: &[]u8, ciphertext: &[]u8, tag: [16]u8, out_pt: []u8): bool;
}

export fn agcmZeroBlock(): [16]u8;
export fn agcmMakeCounterBlock(nonce: [12]u8, counter: u32): [16]u8;
export fn agcmCtEq16(a: [16]u8, b: [16]u8): bool;
export fn agcmComputeTag(h: [16]u8, e_j0: [16]u8, aad: &[]u8, ct: &[]u8): [16]u8;
```

## Minimal Pattern

```typescript
import { Aes256Gcm } from "aes-gcm";

const gcm = Aes256Gcm.new(key32);
let ct: [64]u8 = undefined;
const tag = gcm.encrypt(nonce12, aad, plaintext, ct[0..plaintext.len]);

let pt: [64]u8 = undefined;
const ok = gcm.decrypt(nonce12, aad, ct[0..plaintext.len], tag, pt[0..plaintext.len]);
```

## Pitfalls

1. **Fixed array inputs**: `key` is `[16]u8` (AES-128) or `[32]u8` (AES-256), `nonce` is `[12]u8`, and `tag` is `[16]u8` — pass fixed arrays, not slices.
2. **Output slice sizing**: `out_ct.len` must be at least `plaintext.len`, and `out_pt.len` at least `ciphertext.len`.
3. **Never reuse `(key, nonce)`**: Nonce reuse in AES-GCM catastrophically reveals the GHASH key `H` and XOR of plaintexts.
4. **Always check `decrypt()`**: Returns `bool`; if `false`, authentication failed and `out_pt` must not be trusted.
