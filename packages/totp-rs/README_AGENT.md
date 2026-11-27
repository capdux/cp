# totp-rs — AI Agent Quick Ref

Zero-allocation Google Authenticator otpauth:// TOTP/HOTP Key URI parser, Base32 secret validator, and builder for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    OtpAuthUri,
    LabelParts,
    otpIsUnreserved,
    otpParseStrictU64,
    otpWriteRaw,
    otpWritePctEncoded,
    otpWriteU64,
    isValidBase32Secret,
} from "totp-rs";
```

## Core Types & Constants

```cp
class OtpAuthUri { kind: u8, label: string, secret: string, issuer: string, /* +5 fields */ }
class LabelParts { issuer: string, account: string }
```

## API Signatures

```cp
fn otpIsUnreserved(c: u8): bool;
fn otpParseStrictU64(s: string): u64?;
fn otpWriteRaw(out: []u8, pos: &mut usize, src: string): bool;
fn otpWritePctEncoded(out: []u8, pos: &mut usize, src: string, allow_label_chars: bool): bool;
fn otpWriteU64(out: []u8, pos: &mut usize, val: u64): bool;
fn isValidBase32Secret(secret: string): bool;
fn parseOtpAuthUri(uri: string): OtpAuthUri;
fn formatOtpAuthUri(cfg: OtpAuthUri, out: []u8): usize;
fn splitLabel(label: string): LabelParts;
```

## Minimal Usage

```cp
import { isValidBase32Secret } from "totp-rs";

pub fn main() {
    let result = isValidBase32Secret("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"totp-rs"`; install via `cap add totp-rs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
