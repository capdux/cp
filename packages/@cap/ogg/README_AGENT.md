# @cap/ogg — AI Agent Quick Ref

Xiph libogg 1.3.5 bitstream container framing and page muxing engine for CP (`v1.3.5`, `BSD-3-Clause`)

## Import

```cp
import {
    OggError,
    OggStream,
    cp_ogg_stream_init,
    cp_ogg_stream_packetin,
    cp_ogg_stream_pageout,
    cp_ogg_stream_flush,
    cp_ogg_stream_clear,
} from "@cap/ogg";
```

## Core Types & Constants

```cp
error OggError { InitFailed, InvalidState, PacketFailed, BufferTooSmall, PageFailed }
class OggStream { raw: *auto!?, serialno: i32 } // methods: init(...) +4
```

## API Signatures

```cp
fn cp_ogg_stream_init(serialno: i32): *auto!?;
fn cp_ogg_stream_packetin(os: *auto!?, data: [*]&u8!, len: isize, bos: i32, eos: i32, granulepos: i64, packetno: i64): i32;
fn cp_ogg_stream_pageout(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32;
fn cp_ogg_stream_flush(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32;
fn cp_ogg_stream_clear(os: *auto!?);
```

## Minimal Usage

```cp
import { cp_ogg_stream_init } from "@cap/ogg";

pub fn main() {
    let result = cp_ogg_stream_init(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/ogg"`; install via `cap add @cap/ogg`.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
