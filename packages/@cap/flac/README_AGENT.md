# @cap/flac — AI Agent Quick Ref

Xiph libFLAC 1.4.3 lossless audio encoder and decoder engine for CP (`v1.4.3`, `BSD-3-Clause`)

## Import

```cp
import {
    FlacError,
    cp_flac_version,
    cp_flac_vendor,
    cp_flac_encode_pcm16_test,
    version,
    vendor,
    encodePcm16Test,
} from "@cap/flac";
```

## Core Types & Constants

```cp
error FlacError { InvalidInput, InitFailed, EncodeFailed }
```

## API Signatures

```cp
fn cp_flac_version(): [*:0]&u8!?;
fn cp_flac_vendor(): [*:0]&u8!?;
fn cp_flac_encode_pcm16_test(pcm: [*]&i16!, samples: u32, sample_rate: u32, out_bytes_written: *u64!): i32;
fn version(): string;
fn vendor(): string;
fn encodePcm16Test(pcm: &[]i16, sample_rate: u32): !u64;
```

## Minimal Usage

```cp
import { version } from "@cap/flac";

pub fn main() {
    let result = version();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/flac"`; install via `cap add @cap/flac`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
