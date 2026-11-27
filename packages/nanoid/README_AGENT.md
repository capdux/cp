# nanoid — AI Agent Quick Ref

- **Package**: `nanoid` (`1.0.0`) | **Source**: npm `nanoid` v5 (zero-heap, bitmask-unbiased)
- **Import**: `import { URL_ALPHABET, DEFAULT_SIZE, nanoidFromBytes, customAlphabetFromBytes, NanoIdGenerator, nanoidSeeded, isValidNanoId } from "nanoid";`
- **Allocation**: Zero-heap; writes into caller `[]u8` and returns actual written byte count.

## Constants & Core Signatures

```typescript
export const URL_ALPHABET: string = "useandom-26T198340PX75pxJACKVERYMINDBUSHWOLF_GQZbfghjklqvwyzrict";
export const DEFAULT_SIZE: usize = 21;

export fn nanoidFromBytes(bytes: []u8, size: usize, out: []u8): usize;
export fn customAlphabetFromBytes(alphabet: string, bytes: []u8, size: usize, out: []u8): usize;
export fn nanoidSeeded(seed: u64, size: usize, out: []u8): usize;
export fn isValidNanoId(id: string, alphabet: string): bool;

export class NanoIdGenerator {
    state: u64,
    fn new(seed: u64): NanoIdGenerator;
    fn nextInto(_: auto, size: usize, out: []u8): usize;
    fn nextCustomInto(_: auto, alphabet: string, size: usize, out: []u8): usize;
}

// Low-level helpers
export fn nidStepState(state: u64): u64;
export fn nidBitmask(alphabet_len: usize): u8;
export fn nidContainsByte(alphabet: string, ch: u8): bool;
```

## Minimal Pattern

```typescript
import { NanoIdGenerator, DEFAULT_SIZE } from "nanoid";

let gen = NanoIdGenerator.new(12345);
let out: [DEFAULT_SIZE]u8 = undefined;
const n = gen.nextInto(DEFAULT_SIZE, out[0..]);
```

## Pitfalls

1. **Security**: `NanoIdGenerator` / `nanoidSeeded` use a non-cryptographic PRNG; for security tokens, feed OS CSPRNG bytes into `nanoidFromBytes` or `customAlphabetFromBytes`.
2. **Extra entropy for rejection sampling**: `customAlphabetFromBytes` may reject bytes outside `alphabet.len`; provide extra random bytes (e.g., `2 * size`) and verify returned length equals `size`.
3. **Alphabet length**: `alphabet` must be non-empty and `<= 256` bytes.
4. **Output buffer**: `out.len` must be `>= size`, otherwise output is truncated to `out.len`.
