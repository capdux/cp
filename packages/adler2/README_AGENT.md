# adler2 — AI Agent Quick Ref

Zero-heap RFC 1950 Adler-32 checksum with streaming, O(1) rolling window, and chunk combine in pure CP (crates.io `adler2` parity).

## Import

```cp
import {
    ADLER32_MOD, ADLER32_NMAX,
    Adler32,
    adler32, adler32Slice, adler32Str,
    combineAdler32, adler32Combine,
} from "adler2";
```

## API Signatures

```cp
const ADLER32_MOD: u32;    // 65521
const ADLER32_NMAX: usize; // 5552

class Adler32 {
    s1: u32, s2: u32,
    static fn init(): Adler32;
    static fn new(): Adler32;
    static fn fromValue(value: u32): Adler32;
    static fn fromChecksum(value: u32): Adler32;
    fn update(*self, data: []const u8);
    fn writeSlice(*self, data: []const u8);
    fn writeStr(*self, s: string);
    fn updateByte(*self, b: u8);
    fn writeByte(*self, b: u8);
    fn roll(*self, window_len: usize, out_byte: u8, in_byte: u8);
    fn finalize(): u32;
    fn checksum(): u32;
    fn reset(*self);
}

fn adler32(data: []const u8): u32;
fn adler32Slice(data: []const u8): u32;
fn adler32Str(s: string): u32;
fn combineAdler32(adler1: u32, adler2: u32, len2: u64): u32;
fn adler32Combine(adler1: u32, adler2: u32, len2: u64): u32;
```

## Minimal Example

```cp
import { Adler32, adler32Str, combineAdler32 } from "adler2";

let h = Adler32.new();
h.writeStr("Wikipedia");
let c = h.checksum(); // 0x11e60398
let merged = combineAdler32(adler32Str("Wiki"), adler32Str("pedia"), 5);
```

## Pitfalls & Borrowing Rules

- **Aliases**: `init`/`new`, `fromValue`/`fromChecksum`, `update`/`writeSlice`, `updateByte`/`writeByte`, `finalize`/`checksum`, `adler32`/`adler32Slice`, and `combineAdler32`/`adler32Combine` are identical pairs.
- **Non-destructive read**: `finalize()` and `checksum()` do not reset the hasher; call `reset()` explicitly to reuse an `Adler32` instance.
- **Rolling window**: `roll(window_len, out_byte, in_byte)` requires `window_len` to match the exact byte count currently hashed in the sliding window.
- **Zero-copy**: All slice inputs are read-only borrows (`[]const u8`).
