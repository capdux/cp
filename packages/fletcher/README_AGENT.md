# fletcher — AI Agent Quick Ref

- **Import**: `import { Fletcher16, Fletcher32, Fletcher64, fletcher16, fletcher16Bytes, calcFletcher16, fletcher32, fletcher32Bytes, calcFletcher32, fletcher64, fletcher64Bytes, calcFletcher64 } from "fletcher";`
- **Source**: crates.io `fletcher` (`v1.0.0`, zero-heap)

## Core Signatures
```cp
class Fletcher16 {
    sum1: u32, sum2: u32,
    static new(): Fletcher16;                              // alias: init()
    static withInitialValues(a: u8, b: u8): Fletcher16;
    update(data: string);
    updateBytes(data: &[]u8);
    digest(): u16;                                         // alias: value()
    reset();
}
class Fletcher32 {
    sum1: u32, sum2: u32, pending: u8, has_pending: bool,
    static new(): Fletcher32;                              // alias: init()
    static withInitialValues(a: u16, b: u16): Fletcher32;
    updateWords(words: &[]u16);
    updateBytes(data: &[]u8);
    update(data: string);
    digest(): u32;                                         // alias: value()
    reset();
}
class Fletcher64 {
    sum1: u64, sum2: u64, pending: [4]u8, pending_len: usize,
    static new(): Fletcher64;                              // alias: init()
    static withInitialValues(a: u32, b: u32): Fletcher64;
    updateWords(words: &[]u32);
    updateBytes(data: &[]u8);
    update(data: string);
    digest(): u64;                                         // alias: value()
    reset();
}

// One-shot helpers
fletcher16(data: string): u16;     fletcher16Bytes(data: &[]u8): u16;   calcFletcher16(data: &[]u8): u16;
fletcher32(data: string): u32;     fletcher32Bytes(data: &[]u8): u32;   calcFletcher32(words: &[]u16): u32;
fletcher64(data: string): u64;     fletcher64Bytes(data: &[]u8): u64;   calcFletcher64(words: &[]u32): u64;
```

## Pitfalls
- Bind streaming hashers with `let mut h = Fletcher32.new()` because `update*`/`reset` mutate state.
- `calcFletcher32` takes `&[]u16` and `calcFletcher64` takes `&[]u32` (matching Rust `fletcher`); for raw bytes use `fletcher32Bytes` / `fletcher64Bytes`.
- Calling `digest()`/`value()` on `Fletcher32`/`Fletcher64` does not consume pending bytes permanently, so you may inspect intermediate checksums or continue feeding bytes.
- Fletcher checksums are for error detection, not cryptographic integrity.
