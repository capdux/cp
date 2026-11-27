# @cap/libavif — AI Agent Quick Ref

AOMediaCodec libavif (v1.2.1) AVIF/HEIF image container & pixel format library for CP (`v1.2.1`, `BSD-2-Clause`)

## Import

```cp
import {
    AvifError,
    AvifImage,
    cp_avif_version,
    cp_avif_pixel_format_to_string,
    cp_avif_image_create,
    cp_avif_image_destroy,
    cp_avif_image_width,
    cp_avif_image_height,
} from "@cap/libavif";
```

## Core Types & Constants

```cp
error AvifError { InvalidArgument, AllocFailed }
class AvifImage { raw: *auto!? } // methods: create(...) +4
```

## API Signatures

```cp
fn cp_avif_version(): [*:0]&u8!?;
fn cp_avif_pixel_format_to_string(fmt: i32): [*:0]&u8!?;
fn cp_avif_image_create(width: u32, height: u32, depth: u32, yuv_format: i32): *auto!?;
fn cp_avif_image_destroy(img: *auto!?);
fn cp_avif_image_width(img: *auto!?): u32;
fn cp_avif_image_height(img: *auto!?): u32;
fn cp_avif_image_depth(img: *auto!?): u32;
fn cp_avif_image_create_and_destroy(width: u32, height: u32, depth: u32): i32;
fn version(): string;
fn pixelFormatName(fmt: i32): string;
fn imageCreateAndDestroy(width: u32, height: u32, depth: u32): !void;
```

## Minimal Usage

```cp
import { version } from "@cap/libavif";

pub fn main() {
    let result = version();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/libavif"`; install via `cap add @cap/libavif`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
