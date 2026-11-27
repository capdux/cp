# bech32 — AI Agent Quick Ref

Zero-allocation BIP-173 Bech32 and BIP-350 Bech32m encoder, decoder, 8-to-5 bit converter, and BCH checksum verifier for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Bech32Variant,
    Bech32Decoded,
    ParsedBech32Meta,
    charsetIndex,
    polymodStep,
    polymodHrp,
    variantConst,
    isValidHrp,
} from "bech32";
```

## Core Types & Constants

```cp
enum Bech32Variant { Bech32, Bech32m }
class Bech32Decoded { valid: bool, variant: Bech32Variant, hrp: string, data: string } // methods: invalid(...)
class ParsedBech32Meta { valid: bool, variant: Bech32Variant, sep: usize }
```

## API Signatures

```cp
fn charsetIndex(c: u8): u8?;
fn polymodStep(chk: u32, v: u8): u32;
fn polymodHrp(hrp: string): u32;
fn variantConst(v: Bech32Variant): u32;
fn isValidHrp(hrp: string): bool;
fn to5BitWords(bytes: string, out: []u8): usize?;
fn from5BitWords(words: &[]u8, out: []u8): usize?;
fn encodeBech32(hrp: string, payload_bytes: string, variant: Bech32Variant, out: []u8): string?;
fn validateStructureAndChecksum(encoded: string): ParsedBech32Meta;
fn verifyBech32(encoded: string): bool;
fn decodeBech32(encoded: string, hrp_out: []u8, data_out: []u8): Bech32Decoded;
```

## Minimal Usage

```cp
import { isValidHrp } from "bech32";

pub fn main() {
    let result = isValidHrp("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bech32"`; install via `cap add bech32`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
