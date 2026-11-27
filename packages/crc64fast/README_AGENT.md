# crc64fast — AI Agent Quick Reference

- **Package**: `crc64fast` (`v1.0.0`) | **Source**: crates.io `crc64fast` | **Import**: `import { ... } from "crc64fast";`
- **Role**: Zero-heap 64-bit CRC (ECMA-182, XZ, GO-ISO) with comptime Slice-by-4 (`[4][256]u64`) LUT & streaming hasher.

## Core Exports (`src/index.d.cp`)

```cp
class Crc64Hasher {
    table: [256]u64,
    crc: u64,
    init_val: u64,
    xorout: u64,
    reflected: bool,
    preset: u8,
    fn initCustom(poly: u64, init_val: u64, xorout: u64, reflected: bool): Crc64Hasher;
    fn initEcma(): Crc64Hasher;
    fn initXz(): Crc64Hasher;
    fn initIso(): Crc64Hasher;
    fn update(_: auto, data: &[]u8);
    fn finalize(): u64;
    fn reset(_: auto);
}

fn crc64Ecma(data: &[]u8): u64;
fn crc64Xz(data: &[]u8): u64;
fn crc64Iso(data: &[]u8): u64;
fn makeU64(hi: u32, lo: u32): u64;
```

## Minimal Usage

```cp
import { Crc64Hasher, crc64Ecma, crc64Xz, crc64Iso } from "crc64fast";

const data: []const u8 = "123456789";
const e = crc64Ecma(&data); // 0x6C40DF5F0B497347
const x = crc64Xz(&data);   // 0x995DC9BBDF1939FA
const i = crc64Iso(&data);  // 0xB90956C775A41001

let h = Crc64Hasher.initXz();
h.update(&"1234");
h.update(&"56789");
const s = h.finalize();     // 0x995DC9BBDF1939FA
```

## Pitfalls & Constraints

1. **Preset fast-path**: `initEcma()`, `initXz()`, `initIso()` use comptime Slice-by-4 `[4][256]u64` LUTs (4 bytes/iter) with zero table-build cost. `initCustom(...)` builds a 256-entry LUT at runtime (1 byte/iter).
2. **Reflected vs normal polynomial**: For `initCustom(..., reflected: true)`, pass the **bit-reflected** polynomial (e.g. `0xC96C5795D7870F42` for XZ); for `reflected: false`, pass the normal polynomial.
3. **Non-destructive `finalize()`**: `finalize()` returns `crc ^ xorout` without mutating internal state; call `reset()` before hashing an unrelated message.
4. **Input type**: Takes `&[]u8` byte slices.
