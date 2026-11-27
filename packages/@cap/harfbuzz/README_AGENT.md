# @cap/harfbuzz — AI Agent Quick Ref

HarfBuzz (v15.2.0) industrial-standard OpenType text shaping engine (CJK/Arabic/Latin/Indic) for CP (`v15.2.0`, `MIT`)

## Import

```cp
import {
    HarfBuzzError,
    HbBuffer,
    cp_hb_version_string,
    cp_hb_buffer_create,
    cp_hb_buffer_destroy,
    cp_hb_buffer_reset,
    cp_hb_buffer_add_utf8,
    cp_hb_buffer_guess_segment_properties,
} from "@cap/harfbuzz";
```

## Core Types & Constants

```cp
error HarfBuzzError { BufferCreateFailed }
class HbBuffer { raw: *auto!? } // methods: create(...) +6
```

## API Signatures

```cp
fn cp_hb_version_string(): [*:0]&u8!;
fn cp_hb_buffer_create(): *auto!?;
fn cp_hb_buffer_destroy(buf: *auto!?);
fn cp_hb_buffer_reset(buf: *auto!?);
fn cp_hb_buffer_add_utf8(buf: *auto!?, text: [*]&u8!, len: i32);
fn cp_hb_buffer_guess_segment_properties(buf: *auto!?);
fn cp_hb_buffer_get_length(buf: *auto!?): u32;
fn cp_hb_buffer_get_direction(buf: *auto!?): i32;
fn versionString(): string;
```

## Minimal Usage

```cp
import { versionString } from "@cap/harfbuzz";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/harfbuzz"`; install via `cap add @cap/harfbuzz`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
