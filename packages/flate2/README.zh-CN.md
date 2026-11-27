[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# flate2

纯 CP 实现的零堆分配 **RFC 1952 Gzip** 与 **RFC 1950 Zlib** Stored-DEFLATE 帧编解码库，内置严格的 **CRC-32**、**ISIZE** 与 **Adler-32** 完整性校验。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [crates.io](https://crates.io/crates/flate2) |
| **原包名与版本** | `flate2`（v1 对齐） |
| **上游仓库** | https://github.com/rust-lang/flate2-rs |
| **CP 包名与版本** | `flate2`（`1.0.0`） |
| **开源协议** | MIT / Apache-2.0 |

在 CP 中保持原包名直接导入，无需改名：

```rust
// Rust (crates.io)
use flate2::write::{GzEncoder, ZlibEncoder};
use flate2::read::{GzDecoder, ZlibDecoder};
```

```typescript
// CP (cap)
import {
    encodeGzipStored,
    decodeGzipStored,
    encodeZlibStored,
    decodeZlibStored,
    parseGzipHeader,
} from "flate2";
```

## 安装

```bash
cap add flate2
```

## 核心特性与安全亮点

- **零堆分配**：所有编解码均写入调用方提供的 `[]u8` 缓冲区，返回 `usize?`（缓冲区不足、帧截断或校验失败时返回 `null`）。
- **严格完整性校验**：
  - Gzip（RFC 1952）：校验魔数、压缩方法、可选头字段（FEXTRA/FNAME/FCOMMENT/FHCRC）、DEFLATE Stored 块 `LEN/NLEN` 补码、CRC-32 与 ISIZE。
  - Zlib（RFC 1950）：校验 CMF/FLG 头校验和（模 31 为 0），拒绝不支持的预设字典，校验大端序 Adler-32 尾部。
- **确定性尺寸预测**：可通过 `fl2StoredEncodedLen(payload_len)` 提前精确计算 Stored DEFLATE 块占用字节数。

## 快速上手

### Gzip 编码、头部解析与解码

```typescript
import {
    encodeGzipStored,
    decodeGzipStored,
    parseGzipHeader,
} from "flate2";

fn main(): void {
    let payload = "Hello flate2 in CP!";
    let frame_buf: [256]u8 = undefined;

    const encoded_len = encodeGzipStored(payload, 1700000000, frame_buf[0..]);
    if (encoded_len == null) {
        return;
    }
    const frame = frame_buf[0..encoded_len.?];

    const hdr = parseGzipHeader(frame);
    if (!hdr.valid) {
        return;
    }

    let out_buf: [256]u8 = undefined;
    const decoded_len = decodeGzipStored(frame, out_buf[0..]);
    if (decoded_len == null) {
        return;
    }
}
```

### Zlib 编解码

```typescript
import { encodeZlibStored, decodeZlibStored } from "flate2";

fn roundtripZlib(payload: []u8, tmp: []u8, out: []u8): usize? {
    const zlen = encodeZlibStored(payload, tmp);
    if (zlen == null) {
        return null;
    }
    return decodeZlibStored(tmp[0..zlen.?], out);
}
```

## API 速览

| 导出符号 | 签名 / 值 | 说明 |
| :--- | :--- | :--- |
| `GZIP_ID1` / `GZIP_ID2` / `GZIP_CM_DEFLATE` | `31` / `139` / `8` | RFC 1952 Gzip 帧头常量 |
| `ZLIB_CMF_DEFAULT` / `ZLIB_FLG_NO_COMP` | `120` / `1` | RFC 1950 Zlib 帧头常量 |
| `GzipHeader` | `{ method, flags, mtime, os, data_offset, valid }` | 解析后的 Gzip 头部结构 |
| `parseGzipHeader` | `(frame: []u8) -> GzipHeader` | 解析 Gzip 头部并定位数据偏移 |
| `encodeGzipStored` | `(payload: []u8, mtime: u32, out: []u8) -> usize?` | 编码 RFC 1952 Gzip 帧 |
| `decodeGzipStored` | `(frame: []u8, out: []u8) -> usize?` | 解码 Gzip 帧并验证 CRC-32 与 ISIZE |
| `encodeZlibStored` | `(payload: []u8, out: []u8) -> usize?` | 编码 RFC 1950 Zlib 帧 |
| `decodeZlibStored` | `(frame: []u8, out: []u8) -> usize?` | 解码 Zlib 帧并验证 Adler-32 |
| `fl2StoredEncodedLen` | `(payload_len: usize) -> usize` | 计算 Stored DEFLATE 块编码字节数 |

## 开源协议

MIT / Apache-2.0
