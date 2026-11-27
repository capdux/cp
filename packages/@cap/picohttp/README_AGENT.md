# @cap/picohttp — AI Agent Quick Ref

CP native package `@cap/picohttp`. (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
error PicoHttpError { OutOfMemory, NotInitialized, ParseFailed, Incomplete, InvalidChunkedEncoding }
class phr_header { name: [*]&u8!?, name_len: usize, value: [*]&u8!?, value_len: usize }
class phr_chunked_decoder { bytes_left_in_chunk: usize, consume_trailer: u8, _hex_count: u8, _state: u8 }
class ParsedHeader { name: &[]u8, value: &[]u8 }
class ParsedRequest { method: &[]u8, path: &[]u8, minor_version: i32, header_count: usize, /* +1 fields */ }
class ParsedResponse { minor_version: i32, status: i32, message: &[]u8, header_count: usize, /* +1 fields */ }
class HttpParser { headers_ptr: [*]phr_header!?, max_headers: usize, allocator: std.mem.Allocator } // methods: init(...) +4
class ChunkedDecoder { inner: phr_chunked_decoder } // methods: init(...) +3
```

## API Signatures

```cp
fn phr_parse_request(buf: [*]&u8!, len: usize, method: *[*]&u8!?!, method_len: *usize!, path: *[*]&u8!?!, path_len: *usize!, minor_version: *i32!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32;
fn phr_parse_response(buf: [*]&u8!, len: usize, minor_version: *i32!, status: *i32!, msg: *[*]&u8!?!, msg_len: *usize!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32;
fn phr_decode_chunked(decoder: *phr_chunked_decoder!, buf: [*]u8!, bufsz: *usize!): isize;
fn phr_decode_chunked_is_in_data(decoder: *phr_chunked_decoder!): i32;
```

## Minimal Usage

```cp
import { phr_decode_chunked_is_in_data } from "@cap/picohttp";

pub fn main() {
    let result = phr_decode_chunked_is_in_data(/* decoder: *phr_chunked_decoder! */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/picohttp"`; install via `cap add @cap/picohttp`.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
