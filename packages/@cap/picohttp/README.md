[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/picohttp

Native CP package `@cap/picohttp`.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`picohttpparser`](https://github.com/h2o/picohttpparser)（`MIT`）（原库名 `picohttpparser`，CP 包名 `@cap/picohttp`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/picohttp` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/picohttp
```

```cp
import {
    PicoHttpError,
    phr_header,
    phr_chunked_decoder,
    ParsedHeader,
    ParsedRequest,
    ParsedResponse,
    HttpParser,
    ChunkedDecoder,
} from "@cap/picohttp";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `PicoHttpError` | `OutOfMemory, NotInitialized, ParseFailed, Incomplete, InvalidChunkedEncoding` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `phr_header` | `class` | Fields: `name: [*]&u8!?, name_len: usize, value: [*]&u8!?, value_len: usize` |
| `phr_chunked_decoder` | `class` | Fields: `bytes_left_in_chunk: usize, consume_trailer: u8, _hex_count: u8, _state: u8` |
| `ParsedHeader` | `class` | Fields: `name: &[]u8, value: &[]u8` |
| `ParsedRequest` | `class` | Fields: `method: &[]u8, path: &[]u8, minor_version: i32, header_count: usize, bytes_read: usize` |
| `ParsedResponse` | `class` | Fields: `minor_version: i32, status: i32, message: &[]u8, header_count: usize, bytes_read: usize` |
| `HttpParser` | `class` | Fields: `headers_ptr: [*]phr_header!?, max_headers: usize, allocator: std.mem.Allocator` · Methods: `init, parseRequest, parseResponse, headerAt, __cp_deinit` |
| `ChunkedDecoder` | `class` | Fields: `inner: phr_chunked_decoder` · Methods: `init, decode, isInData, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `phr_parse_request` | `(buf: [*]&u8!, len: usize, method: *[*]&u8!?!, method_len: *usize!, path: *[*]&u8!?!, path_len: *usize!, minor_version: *i32!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32` | Invokes `phr_parse_request` and returns `i32`. |
| `phr_parse_response` | `(buf: [*]&u8!, len: usize, minor_version: *i32!, status: *i32!, msg: *[*]&u8!?!, msg_len: *usize!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32` | Invokes `phr_parse_response` and returns `i32`. |
| `phr_decode_chunked` | `(decoder: *phr_chunked_decoder!, buf: [*]u8!, bufsz: *usize!): isize` | Invokes `phr_decode_chunked` and returns `isize`. |
| `phr_decode_chunked_is_in_data` | `(decoder: *phr_chunked_decoder!): i32` | Invokes `phr_decode_chunked_is_in_data` and returns `i32`. |

## Minimal Example

```cp
import { phr_decode_chunked_is_in_data } from "@cap/picohttp";

pub fn main() {
    let result = phr_decode_chunked_is_in_data(/* decoder: *phr_chunked_decoder! */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
