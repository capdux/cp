# crc32fast@1.0.0 — AI Agent Quick Ref

- **Import**: `import { Crc32Hasher, hashBytes, hashStr, combineCrc32 } from "crc32fast";`
- **Provenance**: crates.io `crc32fast` (v1) parity | IEEE 802.3 (`0xEDB88320`) | Zero-heap | License: MIT
- **Perf (ReleaseFast)**: `hashBytes` (64 KB) = **1,137.07 MB/s (1.14 GB/s)** (54.97 µs/op, Slice-by-4 LUT)
- **Constant**: `CRC32_POLY_REVERSED: u32 = 0xEDB88320`
- **Types**: `src/index.d.cp`

## Core API
```cp
export fn hashBytes(buf: &[]u8): u32;
export fn hashStr(s: string): u32;
export fn combineCrc32(crc1: u32, crc2: u32, len2: u64): u32;

export class Crc32Hasher {
    state: u32,
    amount: u64,
    fn new(): Crc32Hasher;
    fn newWithInitial(init_crc: u32): Crc32Hasher;
    fn update(_: auto, buf: &[]u8);
    fn updateStr(_: auto, s: string);
    fn finalize(): u32;
    fn reset(_: auto);
    fn combine(_: auto, other: Crc32Hasher);
}
```
- Low-level GF(2)/table helpers: `crcByteStep`, `crcBuildTables`, `crcGf2MatrixTimes`, `crcGf2MatrixSquare`.

## Standard Check Vector
- `hashStr("123456789") == 0xCBF43926`
- `hashStr("") == 0x00000000`

## Minimal Example
```cp
import { Crc32Hasher, hashStr, combineCrc32 } from "crc32fast";

let h = Crc32Hasher.new();
h.updateStr("1234");
h.updateStr("56789");
const sum = h.finalize(); // 0xCBF43926

const combined = combineCrc32(hashStr("1234"), hashStr("56789"), 5); // 0xCBF43926
```

## Gotchas
1. This is **IEEE 802.3 CRC-32** (`0xEDB88320`, used by ZIP/gzip/PNG/Ethernet). For Castagnoli CRC-32C (LevelDB/RocksDB/GCS/iSCSI), use `crc32c`.
2. `finalize()` does **not** reset internal `state`; you can continue calling `update()` / `updateStr()` or call `reset()` to start fresh.
3. `combineCrc32(crc1, crc2, len2)` requires the exact byte length `len2` of the second chunk; `h1.combine(h2)` uses `h2.amount` automatically.
4. `update()` takes `&[]u8` (immutable byte slice); use `updateStr()` for `string`.
