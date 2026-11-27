# @cap/curl — AI Agent Quick Ref

libcurl (v8.23.0) multiprotocol network transfer library (HTTP/FTP/Proxy/Multipart) for CP via prebuilt dynamic library (`v8.23.0`, `curl`)

## Import

```cp
import {
    CurlError,
    CurlResponse,
    cp_curl_version,
    cp_curl_global_init,
    cp_curl_global_cleanup,
    cp_curl_http_get,
    cp_curl_free,
    version,
} from "@cap/curl";
```

## Core Types & Constants

```cp
error CurlError { InitFailed, UrlTooLong, RequestFailed }
class CurlResponse { status: i64, ptr: [*]u8!?, len: usize } // methods: statusCode(...) +3
```

## API Signatures

```cp
fn cp_curl_version(): [*:0]&u8!;
fn cp_curl_global_init(): i32;
fn cp_curl_global_cleanup();
fn cp_curl_http_get(url: [*:0]&u8!, out_status: &mut i64, out_len: &mut usize): [*]u8!?;
fn cp_curl_free(ptr: [*]u8!);
fn version(): string;
fn globalInit(): !void;
fn globalCleanup();
fn httpGet(url: string): !CurlResponse;
```

## Minimal Usage

```cp
import { httpGet } from "@cap/curl";

pub fn main() {
    let result = httpGet("example") catch return;
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/curl"`; install via `cap add @cap/curl`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
