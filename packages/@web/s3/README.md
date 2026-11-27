[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/s3

Official S3 object key sanitizer and pure-CP AWS SigV4 presigned URL endpoint plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/s3` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/s3
```

```cp
import {
    S3PluginConfig,
    S3wSha256Hasher,
    s3wRotr32,
    s3wSha256Compress,
    s3wSha256,
    s3wHmacSha256,
    s3wBytesToHex,
    s3wWriteByte,
} from "@web/s3";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `S3PluginConfig` | `class` | Fields: `endpoint: string, bucket: string, region: string, access_key: string, secret_key: string, presign_path: string, +1 more` |
| `S3wSha256Hasher` | `class` | Fields: `state: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64` · Methods: `init, update, finalize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `s3wRotr32` | `(x: u32, n: u5): u32` | Invokes `s3wRotr32` and returns `u32`. |
| `s3wSha256Compress` | `(state: &mut [8]u32, block: &[64]u8)` | Executes `s3wSha256Compress` in-place. |
| `s3wSha256` | `(data: &[]u8, out: &mut [32]u8)` | Executes `s3wSha256` in-place. |
| `s3wHmacSha256` | `(key: &[]u8, msg: &[]u8, out: &mut [32]u8)` | Executes `s3wHmacSha256` in-place. |
| `s3wBytesToHex` | `(bytes: &[32]u8, out_hex: &mut [64]u8)` | Executes `s3wBytesToHex` in-place. |
| `s3wWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `s3wWriteByte` and returns `bool`. |
| `s3wWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `s3wWriteRaw` and returns `bool`. |
| `s3wWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | Invokes `s3wWriteU64` and returns `bool`. |
| `s3wExtractHost` | `(endpoint: string): string` | Invokes `s3wExtractHost` and returns `string`. |
| `s3wTrimTrailingSlash` | `(s: string): string` | Invokes `s3wTrimTrailingSlash` and returns `string`. |
| `s3wWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `s3wWriteJsonEscaped` and returns `bool`. |
| `s3NormalizeKey` | `(raw_key: string, out_buf: []u8): usize?` | Invokes `s3NormalizeKey` and returns `usize?`. |
| `s3BuildPresignedUrl` | `(cfg: S3PluginConfig, method: string, object_key: string, expires_secs: u64, amz_date: string, out_buf: []u8): usize?` | Invokes `s3BuildPresignedUrl` and returns `usize?`. |
| `s3wPresignHandler` | `(ctx: &mut Context): Response` | Invokes `s3wPresignHandler` and returns `Response`. |
| `createS3Plugin` | `(cfg: S3PluginConfig): Plugin` | Constructs/initializes an instance via `createS3Plugin`. |
| `registerS3` | `(app: &mut App, cfg: S3PluginConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { s3wWriteRaw } from "@web/s3";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = s3wWriteRaw(out_buf[0..256], /* pos: &mut usize */, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
