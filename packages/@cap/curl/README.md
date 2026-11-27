[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/curl

libcurl (v8.23.0) multiprotocol network transfer library (HTTP/FTP/Proxy/Multipart) for CP via prebuilt dynamic library

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`curl`](https://github.com/curl/curl)（`curl`）（包名 `@cap/curl`）。

| Metadata | Value |
| --- | --- |
| Package | `@cap/curl` |
| Version | `8.23.0` |
| License | `curl` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/curl
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `CurlError` | `InitFailed, UrlTooLong, RequestFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CurlResponse` | `class` | Fields: `status: i64, ptr: [*]u8!?, len: usize` · Methods: `statusCode, body, length, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_curl_version` | `(): [*:0]&u8!` | Invokes `cp_curl_version` and returns `[*:0]&u8!`. |
| `cp_curl_global_init` | `(): i32` | Invokes `cp_curl_global_init` and returns `i32`. |
| `cp_curl_global_cleanup` | `()` | Executes `cp_curl_global_cleanup` in-place. |
| `cp_curl_http_get` | `(url: [*:0]&u8!, out_status: &mut i64, out_len: &mut usize): [*]u8!?` | Invokes `cp_curl_http_get` and returns `[*]u8!?`. |
| `cp_curl_free` | `(ptr: [*]u8!)` | Executes `cp_curl_free` in-place. |
| `version` | `(): string` | Invokes `version` and returns `string`. |
| `globalInit` | `(): !void` | Invokes `globalInit` and returns `!void`. |
| `globalCleanup` | `()` | Executes `globalCleanup` in-place. |
| `httpGet` | `(url: string): !CurlResponse` | Invokes `httpGet` and returns `!CurlResponse`. |

## Minimal Example

```cp
import { httpGet } from "@cap/curl";

pub fn main() {
    let result = httpGet("example") catch return;
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
