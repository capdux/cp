# sqids — AI Agent Quick Ref

Zero-allocation YouTube-style reversible short ID generator (Sqids specification) with deterministic shuffle, min-length padding, and profanity blocklist for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SqidsError,
    SqidsOptions,
    Sqids,
    hasDigit,
    isBlockedId,
    shuffleAlphabet,
    reverseBytes,
    appendToId,
} from "sqids";
```

## Core Types & Constants

```cp
enum SqidsError { None, AlphabetTooShort, AlphabetTooLong, AlphabetNonAscii, AlphabetDuplicateChar }
class SqidsOptions { alphabet: string, min_length: u8 }
class Sqids { alphabet: [MAX_ALPHABET_LEN]u8, alphabet_len: usize, min_length: u8, valid: bool, /* +1 fields */ } // methods: initDefault(...) +6
```

## API Signatures

```cp
fn hasDigit(s: string): bool;
fn isBlockedId(id: &[]u8): bool;
fn shuffleAlphabet(buf: []u8);
fn reverseBytes(buf: []u8);
fn appendToId(num: u64, alphabet: &[]u8, dst: []u8, pos: usize): usize?;
fn chunkToNumber(chunk: &[]u8, alphabet: &[]u8): u64?;
```

## Minimal Usage

```cp
import { hasDigit } from "sqids";

pub fn main() {
    let result = hasDigit("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sqids"`; install via `cap add sqids`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
