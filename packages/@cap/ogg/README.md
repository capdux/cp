[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/ogg

Xiph libogg 1.3.5 bitstream container framing and page muxing engine for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/ogg` |
| Version | `1.3.5` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/ogg
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `OggError` | `InitFailed, InvalidState, PacketFailed, BufferTooSmall, PageFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OggStream` | `class` | Fields: `raw: *auto!?, serialno: i32` · Methods: `init, packetIn, pageOut, flush, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_ogg_stream_init` | `(serialno: i32): *auto!?` | Invokes `cp_ogg_stream_init` and returns `*auto!?`. |
| `cp_ogg_stream_packetin` | `(os: *auto!?, data: [*]&u8!, len: isize, bos: i32, eos: i32, granulepos: i64, packetno: i64): i32` | Invokes `cp_ogg_stream_packetin` and returns `i32`. |
| `cp_ogg_stream_pageout` | `(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32` | Invokes `cp_ogg_stream_pageout` and returns `i32`. |
| `cp_ogg_stream_flush` | `(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32` | Invokes `cp_ogg_stream_flush` and returns `i32`. |
| `cp_ogg_stream_clear` | `(os: *auto!?)` | Executes `cp_ogg_stream_clear` in-place. |

## Minimal Example

```cp
import { cp_ogg_stream_init } from "@cap/ogg";

pub fn main() {
    let result = cp_ogg_stream_init(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
