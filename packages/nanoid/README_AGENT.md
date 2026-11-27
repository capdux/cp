# nanoid@1.0.0 (Agent Quickref)

> Zero-heap, URL-safe, bitmask-unbiased unique ID generator with custom alphabet and deterministic PRNG support (clean-room port of npm nanoid v5) | License: MIT
> Import: `import { ... } from "nanoid";` | Full types: `src/index.d.cp`

## Functions
- `export fn nidStepState(state: u64): u64;`
- `export fn nidBitmask(alphabet_len: usize): u8;`
- `export fn nidContainsByte(alphabet: string, ch: u8): bool;`
- `export fn nanoidFromBytes(bytes: []u8, size: usize, out: []u8): usize;`
- `export fn customAlphabetFromBytes(alphabet: string, bytes: []u8, size: usize, out: []u8): usize;`
- `export fn nanoidSeeded(seed: u64, size: usize, out: []u8): usize;`
- `export fn isValidNanoId(id: string, alphabet: string): bool;`
