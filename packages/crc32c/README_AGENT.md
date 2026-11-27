# crc32c@1.0.0 — AI Agent Quick Ref

- **Import**: `import { crc32c, crc32cStr, crc32cAppend, crc32cCombine, maskCrc32c, unmaskCrc32c, Crc32cHasher } from "crc32c";`
- **Provenance**: crates.io `crc32c` (v0.6) parity | Castagnoli (`0x82F63B78`) | Zero-heap | License: MIT
- **Perf (ReleaseFast)**: `crc32c` (64 KB) = **450.59 MB/s** (138.71 µs/op, 256-entry LUT)
- **Constants**: `CRC32C_POLY_REVERSED = 0x82F63B78`, `CRC32C_MASK_DELTA = 0xA282EAD8`
- **Types**: `src/index.d.cp`

## Core API
```cp
export fn crc32c(data: []u8): u32;
export fn crc32cStr(s: string): u32;
export fn crc32cAppend(crc: u32, data: []u8): u32;
export fn crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32;
export fn maskCrc32c(crc: u32): u32;
export fn unmaskCrc32c(masked: u32): u32;

export class Crc32cHasher {
    state: u32,
    amount: u64,
    fn new(): Crc32cHasher;
    fn newWithInitial(init_crc: u32): Crc32cHasher;
    fn update(_: auto, buf: []u8);
    fn updateStr(_: auto, s: string);
    fn finalize(): u32;
    fn reset(_: auto);
    fn combine(_: auto, other: Crc32cHasher);
}
```
- Low-level GF(2) helpers: `c32cByteStep`, `c32cGf2MatrixTimes`, `c32cGf2MatrixSquare`.

## Standard Check Vector
- `crc32cStr("123456789") == 0xE3069283`
- `crc32cStr("") == 0x00000000`

## Minimal Example
```cp
import { crc32cStr, maskCrc32c, unmaskCrc32c, Crc32cHasher } from "crc32c";

let h = Crc32cHasher.new();
h.updateStr("123456789");
const sum = h.finalize();           // 0xE3069283
const m = maskCrc32c(sum);
const orig = unmaskCrc32c(m);       // 0xE3069283
```

## Gotchas
1. This is **Castagnoli CRC-32C** (`0x82F63B78`, used by LevelDB/RocksDB/Snappy/GCS/iSCSI/EXT4). For IEEE 802.3 CRC-32 (ZIP/gzip/PNG), use `crc32fast`.
2. Notice `crc32c(data: []u8)` / `crc32cAppend(crc, data: []u8)` / `Crc32cHasher.update(buf: []u8)` take mutable `[]u8` slices; for `string` literals use `crc32cStr()` or `updateStr()`.
3. `maskCrc32c` / `unmaskCrc32c` implement the exact LevelDB/RocksDB/Snappy formula `((crc >> 15) | (crc << 17)) +% 0xA282EAD8`.
4. `finalize()` does not reset internal state; call `reset()` before reusing the hasher.
