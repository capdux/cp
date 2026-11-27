# @cap/stb — AI Agent Quick Ref

Single-header image decoding (PNG/JPG/BMP/TGA/GIF), PNG encoding and SIMD image resizing (nothings/stb) for CP (`v2.30.0`, `MIT`)

## Import

```cp
import {
    StbError,
    StbImage,
    PngBuffer,
    cp_stbi_load_from_memory,
    cp_stbi_image_free,
    cp_stbi_write_png_to_mem,
    cp_stbiw_free,
    cp_stbir_resize_rgba8,
} from "@cap/stb";
```

## Core Types & Constants

```cp
error StbError { DecodeFailed, EncodeFailed, ResizeFailed, InvalidArg, BufferTooSmall }
class StbImage { raw: [*]u8!?, width: i32, height: i32, channels: i32 } // methods: loadFromMemory(...) +2
class PngBuffer { raw: [*]u8!?, len: usize } // methods: encode(...) +2
```

## API Signatures

```cp
fn cp_stbi_load_from_memory(buffer: [*]&u8!, len: i32, x: *i32!?, y: *i32!?, channels_in_file: *i32!?, desired_channels: i32): [*]u8!?;
fn cp_stbi_image_free(ptr: *auto!?);
fn cp_stbi_write_png_to_mem(pixels: [*]&u8!, w: i32, h: i32, comp: i32, stride_bytes: i32, out_len: *i32!?): [*]u8!?;
fn cp_stbiw_free(ptr: *auto!?);
fn cp_stbir_resize_rgba8(in_pixels: [*]&u8!, in_w: i32, in_h: i32, out_pixels: [*]u8!, out_w: i32, out_h: i32): i32;
fn resizeRgba8(input: &[]u8, in_w: i32, in_h: i32, output: []u8, out_w: i32, out_h: i32): !void;
```

## Minimal Usage

```cp
import { resizeRgba8 } from "@cap/stb";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = resizeRgba8(out_buf[0..256], 0, 0, buf_1[0..256], 0, 0) catch return;
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/stb"`; install via `cap add @cap/stb`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
