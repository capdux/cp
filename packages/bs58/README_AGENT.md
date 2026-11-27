# bs58 — AI Agent Quick Ref

Zero-allocation Base58 encoder/decoder supporting Bitcoin, Ripple, and Flickr alphabets with leading-zero preservation (Rust bs58 / npm bs58 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Base58Alphabet,
    alphabetChars,
    charIndex,
    isValidBase58,
    encodeBase58With,
    decodeBase58With,
    encodeBase58,
    decodeBase58,
} from "bs58";
```

## Core Types & Constants

```cp
enum Base58Alphabet { Bitcoin, Ripple, Flickr }
```

## API Signatures

```cp
fn alphabetChars(alpha: Base58Alphabet): string;
fn charIndex(alphabet: string, c: u8): i8;
fn isValidBase58(encoded: string, alpha: Base58Alphabet): bool;
fn encodeBase58With(input: string, alpha: Base58Alphabet, out: []u8): string?;
fn decodeBase58With(encoded: string, alpha: Base58Alphabet, out: []u8): string?;
fn encodeBase58(input: string, out: []u8): string?;
fn decodeBase58(encoded: string, out: []u8): string?;
```

## Minimal Usage

```cp
import { isValidBase58 } from "bs58";

pub fn main() {
    let result = isValidBase58("example", /* alpha: Base58Alphabet */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bs58"`; install via `cap add bs58`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
