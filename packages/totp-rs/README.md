[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# totp-rs

Zero-allocation Google Authenticator otpauth:// TOTP/HOTP Key URI parser, Base32 secret validator, and builder for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:totp-rs` ([https://github.com/constantoine/totp-rs](https://github.com/constantoine/totp-rs)) / `npm:otpauth` ([https://github.com/hectorm/otpauth](https://github.com/hectorm/otpauth))（原库名 `totp-rs` → CP 包名 `totp-rs`）

| Metadata | Value |
| --- | --- |
| Package | `totp-rs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add totp-rs
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OtpAuthUri` | `class` | Fields: `kind: u8, label: string, secret: string, issuer: string, algorithm: u8, digits: u8, +3 more` |
| `LabelParts` | `class` | Fields: `issuer: string, account: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `otpIsUnreserved` | `(c: u8): bool` | Invokes `otpIsUnreserved` and returns `bool`. |
| `otpParseStrictU64` | `(s: string): u64?` | Invokes `otpParseStrictU64` and returns `u64?`. |
| `otpWriteRaw` | `(out: []u8, pos: &mut usize, src: string): bool` | Invokes `otpWriteRaw` and returns `bool`. |
| `otpWritePctEncoded` | `(out: []u8, pos: &mut usize, src: string, allow_label_chars: bool): bool` | Invokes `otpWritePctEncoded` and returns `bool`. |
| `otpWriteU64` | `(out: []u8, pos: &mut usize, val: u64): bool` | Invokes `otpWriteU64` and returns `bool`. |
| `isValidBase32Secret` | `(secret: string): bool` | Checks predicate `isValidBase32Secret` and returns `bool`. |
| `parseOtpAuthUri` | `(uri: string): OtpAuthUri` | Parses input via `parseOtpAuthUri` and returns `OtpAuthUri`. |
| `formatOtpAuthUri` | `(cfg: OtpAuthUri, out: []u8): usize` | Serializes/formats data via `formatOtpAuthUri` returning `usize`. |
| `splitLabel` | `(label: string): LabelParts` | Invokes `splitLabel` and returns `LabelParts`. |

## Minimal Example

```cp
import { isValidBase32Secret } from "totp-rs";

pub fn main() {
    let result = isValidBase32Secret("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
