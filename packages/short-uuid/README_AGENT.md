# short-uuid — AI Agent Quick Ref

Zero-allocation 128-bit UUID to 22-char Flickr Base58 / Base62 / Cookie Base90 lossless short ID translator for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    parseHexNibble,
    hexNibbleLower,
    uuidToBytes,
    bytesToUuid,
    validateAlphabet,
    findInAlphabet,
    maxEncodedLen,
    shortenUuidWith,
} from "short-uuid";
```

## API Signatures

```cp
fn parseHexNibble(c: u8): u8?;
fn hexNibbleLower(v: u8): u8;
fn uuidToBytes(uuid: string, out16: []u8): bool;
fn bytesToUuid(bytes16: []u8, out36: []u8): string;
fn validateAlphabet(alphabet: string): bool;
fn findInAlphabet(alphabet: string, c: u8): u32?;
fn maxEncodedLen(base: u32): usize;
fn shortenUuidWith(uuid: string, alphabet: string, out: []u8): string?;
fn expandUuidWith(short_id: string, alphabet: string, out36: []u8): string?;
fn shortenUuid(uuid: string, out: []u8): string?;
fn expandUuid(short_id: string, out36: []u8): string?;
```

## Minimal Usage

```cp
import { validateAlphabet } from "short-uuid";

pub fn main() {
    let result = validateAlphabet("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"short-uuid"`; install via `cap add short-uuid`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
