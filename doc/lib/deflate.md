# `cap:deflate` — 纯 CP RFC 1951 DEFLATE / RFC 1952 GZIP / RFC 1950 ZLIB 编解码库

> **模块路径**: `lib/deflate/`  
> **导入协议**: `import { DeflateError, crc32, adler32, compressDeflate, decompressDeflate, decompressDeflateWithMaxSize, compressGzip, decompressGzip, compressZlib, decompressZlib } from "cap:deflate";`  
> **动态挂载**: `import cap from "cap";` -> `cap.deflate.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:deflate` 是 100% 纯 CP 实现、零外部 C 依赖（无 `std.posix` / `@import("bun")` / `std.compress`）的无损数据压缩与容器编解码库，完整覆盖 RFC 1951 (DEFLATE)、RFC 1952 (GZIP) 与 RFC 1950 (ZLIB)：

1. **校验和原语 (`crc32`, `adler32`)**：
   - `crc32(data)`：IEEE 802.3 CRC-32（多项式 `0xedb88320`）。
   - `adler32(data)`：RFC 1950 Adler-32（模 `65521`，分块防溢出累加）。
2. **RFC 1951 Raw DEFLATE (`compressDeflate`, `decompressDeflate`, `decompressDeflateWithMaxSize`)**：
   - **压缩**：32KB 滑动窗口 LZ77 哈希链匹配（最长匹配 `258` 字节）+ Fixed Huffman (`BTYPE=01`) / Stored Block (`BTYPE=00`) 自适应选择。
   - **解压**：完整支持 `BTYPE=00`（Stored）、`BTYPE=01`（Fixed Huffman）与 `BTYPE=10`（Dynamic Huffman）三种块解码，支持重叠距离拷贝，严格拦截 `BTYPE=11`（`InvalidBlockType`）、非法距离（`InvalidDistance`）、损坏哈夫曼码（`InvalidHuffmanCode`）、截断流（`UnexpectedEof`）、尾部多余字节（`TrailingBytes`）与防炸弹输出上限（`OutputTooLarge`）。
3. **RFC 1952 GZIP (`compressGzip`, `decompressGzip`)**：
   - 校验 `1f 8b 08` 魔数与保留位，支持跳过 `FEXTRA`、`FNAME`、`FCOMMENT`、`FHCRC` 可选扩展头，并严格校验 8 字节小端序 `CRC32` 与 `ISIZE`。
4. **RFC 1950 ZLIB (`compressZlib`, `decompressZlib`)**：
   - 校验 `CMF`/`FLG`（`CM=8`、`CINFO<=7`、`(cmf*256+flg) % 31 == 0`、`FDICT=0`）并严格校验 4 字节大端序 `Adler-32`。

---

## 二、 核心 API 速查表 (`lib/deflate/deflate.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `DeflateError` | `UnexpectedEof`, `InvalidBlockType`, `InvalidStoredLength`, `InvalidHuffmanCode`, `InvalidDistance`, `InvalidHeader`, `ChecksumMismatch`, `OutputTooLarge`, `TrailingBytes`, `OutOfMemory` |
| `crc32(data)` | 计算 IEEE 802.3 CRC-32 校验和，返回 `u32` |
| `adler32(data)` | 计算 RFC 1950 Adler-32 校验和，返回 `u32` |
| `compressDeflate(allocator, input)` | 压缩为 RFC 1951 Raw DEFLATE 字节切片（调用方负责 `allocator.free`） |
| `decompressDeflate(allocator, compressed)` | 解压 Raw DEFLATE 字节切片（调用方负责 `allocator.free`） |
| `decompressDeflateWithMaxSize(allocator, compressed, max_bytes)` | 带输出上限 `max_bytes` 的安全解压，超限返回 `DeflateError.OutputTooLarge` |
| `compressGzip(allocator, input)` | 压缩封装为 RFC 1952 GZIP 容器 |
| `decompressGzip(allocator, gz_bytes)` | 解压 GZIP 容器（支持可选头跳过，校验 CRC-32 与 ISIZE） |
| `compressZlib(allocator, input)` | 压缩封装为 RFC 1950 ZLIB 容器 |
| `decompressZlib(allocator, zlib_bytes)` | 解压 ZLIB 容器（校验 CMF/FLG 与大端序 Adler-32） |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    compressDeflate,
    decompressDeflateWithMaxSize,
    compressGzip,
    decompressGzip,
    compressZlib,
    decompressZlib,
} from "cap:deflate";

fn demo(alloc: std.mem.Allocator): !void {
    const raw = "hello deflate! hello deflate! hello deflate!";

    const gz = compressGzip(alloc, raw)?;
    defer alloc.free(gz);
    const gz_restored = decompressGzip(alloc, gz)?;
    defer alloc.free(gz_restored);

    const zl = compressZlib(alloc, raw)?;
    defer alloc.free(zl);
    const zl_restored = decompressZlib(alloc, zl)?;
    defer alloc.free(zl_restored);

    const defl = compressDeflate(alloc, raw)?;
    defer alloc.free(defl);
    const defl_restored = decompressDeflateWithMaxSize(alloc, defl, 4096)?;
    defer alloc.free(defl_restored);
}
```
