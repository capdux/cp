# bytes@1.0.0 — AI Agent Quick Ref

- **Import**: `import { parse, parseBytes, formatBytes, formatIec, formatSi, BytesReader, BytesMut } from "bytes";`
- **Provenance**: npm `bytes` (v3) + crates.io `bytes` (v1) unified superset | Zero-heap | License: MIT
- **Constants**: `KIB`/`MIB`/`GIB`/`TIB`/`PIB` (1024^n), `KB`/`MB`/`GB`/`TB`/`PB` (1000^n), `BYTES_MUT_CAP = 512`
- **Types**: `src/index.d.cp`

## Size Parse & Format
- `parse(input: string): i64` — binary default (`1KB` -> `1024`); returns `-1` on invalid input
- `parseBytes(input: string, binary_default: bool): i64` — explicit binary/SI mode; returns `-1` on error
- `formatBytes(bytes: u64, binary: bool, decimals: u32, out: []u8): usize`
- `formatIec(bytes: u64, out: []u8): usize` — IEC binary (`KiB`/`MiB`/...)
- `formatSi(bytes: u64, out: []u8): usize` — SI decimal (`KB`/`MB`/...)

## Binary Buffer Cursors
```cp
class ReadU8Result  { ok: bool, val: u8 }
class ReadU16Result { ok: bool, val: u16 }
class ReadU32Result { ok: bool, val: u32 }
class ReadU64Result { ok: bool, val: u64 }

class BytesReader {
    data: &[]u8, pos: usize,
    fn new(data: &[]u8): BytesReader;
    fn remaining(): usize;
    fn hasRemaining(): bool;
    fn position(): usize;
    fn advance(_: auto, cnt: usize): bool;
    fn getU8(_: auto): ReadU8Result;
    fn getU16Be(_: auto): ReadU16Result; fn getU16Le(_: auto): ReadU16Result;
    fn getU32Be(_: auto): ReadU32Result; fn getU32Le(_: auto): ReadU32Result;
    fn getU64Be(_: auto): ReadU64Result; fn getU64Le(_: auto): ReadU64Result;
    fn copyToSlice(_: auto, dst: []u8): bool;
}

class BytesMut {
    buf: [512]u8, used: usize,
    fn new(): BytesMut;
    fn len(): usize; fn capacity(): usize; fn remainingMut(): usize; fn isEmpty(): bool;
    fn clear(_: auto); fn truncate(_: auto, new_len: usize); fn byteAt(idx: usize): u8;
    fn putU8(_: auto, v: u8): bool;
    fn putU16Be(_: auto, v: u16): bool; fn putU16Le(_: auto, v: u16): bool;
    fn putU32Be(_: auto, v: u32): bool; fn putU32Le(_: auto, v: u32): bool;
    fn putU64Be(_: auto, v: u64): bool; fn putU64Le(_: auto, v: u64): bool;
    fn putSlice(_: auto, src: &[]u8): bool;
    fn splitTo(_: auto, at: usize, out: []u8): i64;
    fn copyAll(out: []u8): usize;
}
```

## Minimal Example
```cp
import { parse, BytesMut, BytesReader } from "bytes";
const n = parse("2MiB"); // 2097152
let w = BytesMut.new();
_ = w.putU32Be(0xDEADBEEF);
let raw: [16]u8 = undefined;
const len = w.copyAll(raw[0..]);
let r = BytesReader.new(raw[0..len]);
const v = r.getU32Be(); // v.ok == true, v.val == 0xDEADBEEF
```

## Gotchas
1. `parse()` / `parseBytes()` return `i64` with `-1` on invalid input — check `< 0` before casting to `u64`/`usize`.
2. `BytesMut` has a fixed 512-byte inline capacity (`put*` returns `false` if full; `splitTo` returns `-1` if `at > used` or `out.len < at`).
3. Always check `.ok` on `ReadU*Result` before using `.val`.
4. `formatIec`/`formatSi`/`formatBytes` write to `out: []u8` and return bytes written (`>= 32` bytes recommended).
