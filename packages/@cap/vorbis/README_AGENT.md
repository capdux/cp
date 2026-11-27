# @cap/vorbis — AI Agent Quick Ref

Xiph libvorbis 1.3.7 royalty-free audio codec (self-contained with libogg) for CP (`v1.3.7`, `BSD-3-Clause`)

## Import

```cp
import {
    VorbisError,
    cp_vorbis_version_string,
    cp_vorbis_encoder_test_init,
    versionString,
    encoderTestInit,
} from "@cap/vorbis";
```

## Core Types & Constants

```cp
error VorbisError { InvalidParams, InitFailed }
```

## API Signatures

```cp
fn cp_vorbis_version_string(): [*:0]&u8!;
fn cp_vorbis_encoder_test_init(channels: i32, rate: isize, quality: f32): i32;
fn versionString(): string;
fn encoderTestInit(channels: i32, rate: isize, quality: f32): !void;
```

## Minimal Usage

```cp
import { versionString } from "@cap/vorbis";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/vorbis"`; install via `cap add @cap/vorbis`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
