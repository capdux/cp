# `cap:zlib` / `cap.zlib` 标准库参考手册

## 1. 概述

`cap:zlib` 提供纯 CP 实现的工业级 **Zlib (RFC 1950)**、**Gzip (RFC 1952)** 与 **Raw Deflate (RFC 1951)** 统一压缩、解压与校验能力，零外部 C 依赖，内置防压缩炸弹（`max_output_bytes`）与多成员 Gzip 拼接流支持。

## 2. 核心类型

- `ZlibFormat`: `RawDeflate | Zlib | Gzip | AutoDetect`（解码自动识别 `0x1F 0x8B` Gzip 与 `0x78` Zlib）。
- `ZlibLevel`: `None (0) | Fast (1) | Default (6) | Best (9)`。
- `ZlibOptions`:
  - `level: ZlibLevel = ZlibLevel.Default`
  - `max_output_bytes: usize = 64 * 1024 * 1024`（默认 64 MiB 解压上限）
  - `gzip_filename: ([]const u8)? = null`（RFC 1952 `FNAME`）
  - `gzip_mtime: u32 = 0`（RFC 1952 `MTIME`）
  - `gzip_os: u8 = 255`（RFC 1952 `OS`）
  - `gzip_comment: ([]const u8)? = null`（RFC 1952 `FCOMMENT`）
  - `gzip_fhcrc: bool = false`（RFC 1952 `FHCRC`）
- `ZlibError`: `UnexpectedEof`, `InvalidHeader`, `InvalidFormat`, `InvalidBlockType`, `InvalidStoredLength`, `InvalidHuffmanCode`, `InvalidDistance`, `ChecksumMismatch`, `OutputTooLarge`, `TrailingBytes`, `OutOfMemory`。

## 3. API 速查

| 类别 | 函数签名 | 说明 |
| --- | --- | --- |
| 校验 | `crc32(data: []const u8): u32` | IEEE 802.3 CRC-32 |
| 校验 | `adler32(data: []const u8): u32` | RFC 1950 Adler-32 |
| 识别 | `detectFormat(data: []const u8): ZlibFormat ! ZlibError` | 自动识别 Gzip / Zlib / RawDeflate |
| 统一编解码 | `compress(alloc, input, format, options): ([]u8) ! ZlibError` | 统一格式压缩 |
| 统一编解码 | `decompress(alloc, input, format, options): ([]u8) ! ZlibError` | 统一格式解压（支持 `AutoDetect`） |
| 同步便捷 | `gzipSync / gunzipSync` | Gzip (RFC 1952，支持多成员拼接流) |
| 同步便捷 | `deflateSync / inflateSync` | Zlib (RFC 1950) |
| 同步便捷 | `deflateRawSync / inflateRawSync` | Raw Deflate (RFC 1951) |

## 4. 示例

```cp
import cap from "cap";
import { ZlibFormat, ZlibOptions } from "cap:zlib";

const alloc = std.heap.page_allocator;
const opts = ZlibOptions{ .gzip_filename = "data.txt", .gzip_fhcrc = true };
const gz = cap.zlib.gzipSync(alloc, "hello cp zlib", opts) ?;
defer alloc.free(gz);

const plain = cap.zlib.decompress(alloc, gz, ZlibFormat.AutoDetect, ZlibOptions{}) ?;
defer alloc.free(plain);
```
