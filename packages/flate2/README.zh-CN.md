[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# flate2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 1952 Gzip & RFC 1950 Zlib stored-deflate framing codec with strict CRC-32/ISIZE/Adler-32 verification (crates.io flate2 equivalent)

## 安装与导入

```bash
cap add flate2
```

```cp
import { fl2EmptyGzipHeader } from "flate2";
```

## 核心导出 API

- `export fn fl2EmptyGzipHeader(): GzipHeader;`
- `export fn fl2StoredEncodedLen(payload_len: usize): usize;`
- `export fn fl2EncodeStoredBlocks(payload: []u8, out: []u8, start: usize): usize;`
- `export fn fl2DecodeStoredBlocks(stream: []u8, out: []u8): usize?;`
- `export fn parseGzipHeader(frame: []u8): GzipHeader;`
- `export fn encodeGzipStored(payload: []u8, mtime: u32, out: []u8): usize?;`
- `export fn decodeGzipStored(frame: []u8, out: []u8): usize?;`
- `export fn encodeZlibStored(payload: []u8, out: []u8): usize?;`
- `export fn decodeZlibStored(frame: []u8, out: []u8): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`flate2@1.0.0`)，许可证：MIT OR Apache-2.0
