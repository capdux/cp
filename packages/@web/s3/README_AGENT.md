# @web/s3 — AI Agent Quick Ref

Official S3 object key sanitizer and pure-CP AWS SigV4 presigned URL endpoint plugin for cap:web (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class S3PluginConfig { endpoint: string, bucket: string, region: string, access_key: string, /* +3 fields */ }
class S3wSha256Hasher { state: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64 } // methods: init(...) +2
```

## API Signatures

```cp
fn s3wRotr32(x: u32, n: u5): u32;
fn s3wSha256Compress(state: &mut [8]u32, block: &[64]u8);
fn s3wSha256(data: &[]u8, out: &mut [32]u8);
fn s3wHmacSha256(key: &[]u8, msg: &[]u8, out: &mut [32]u8);
fn s3wBytesToHex(bytes: &[32]u8, out_hex: &mut [64]u8);
fn s3wWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn s3wWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn s3wWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;
fn s3wExtractHost(endpoint: string): string;
fn s3wTrimTrailingSlash(s: string): string;
fn s3wWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;
fn s3NormalizeKey(raw_key: string, out_buf: []u8): usize?;
fn s3BuildPresignedUrl(cfg: S3PluginConfig, method: string, object_key: string, expires_secs: u64, amz_date: string, out_buf: []u8): usize?;
fn s3wPresignHandler(ctx: &mut Context): Response;
fn createS3Plugin(cfg: S3PluginConfig): Plugin;
fn registerS3(app: &mut App, cfg: S3PluginConfig): &mut App;
```

## Minimal Usage

```cp
import { s3wWriteRaw } from "@web/s3";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = s3wWriteRaw(out_buf[0..256], /* pos: &mut usize */, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/s3"`; install via `cap add @web/s3`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
