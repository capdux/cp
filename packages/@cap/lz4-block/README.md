# @cap/lz4-block

零堆分配的 CP LZ4 Raw Block 超高速无损压缩与解压引擎 — 洁净室 MIT 实现，对标 Rust `lz4_flex` 与官方 LZ4 Block Format 规范，采用固定 4096 槽栈哈希表（8KB），100% 字节流兼容。

Zero-allocation LZ4 Raw Block lossless compressor & decompressor for CP — clean-room MIT parity with Rust `lz4_flex` and the official LZ4 Block Format.

## 安装 / Install

```bash
cap add @cap/lz4-block
```

## API 参考 / API Reference

| 函数 / Function | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `maxCompressedLen` | `(input_len: usize) -> usize` | 计算最坏情况压缩缓冲区大小 `len + len/255 + 16` |
| `compressBlock` | `(src: []const u8, dst: []u8) -> i64` | 压缩为标准 LZ4 Raw Block，缓冲不足返回 `-1` |
| `decompressBlock` | `(src: []const u8, dst: []u8) -> i64` | 解压 LZ4 Raw Block（支持重叠 RLE 匹配），非法返回 `-1` |
| `compressWithSize` | `(src: []const u8, dst: []u8) -> i64` | 4 字节小端原长度前缀 + LZ4 Block |
| `decompressWithSize` | `(src: []const u8, dst: []u8) -> i64` | 解压并严格校验还原长度与 4 字节头一致 |

## 使用示例 / Usage

```cp
import { compressWithSize, decompressWithSize } from "@cap/lz4-block";

let comp_buf: [256]u8 = undefined;
let out_buf: [256]u8 = undefined;
const n = compressWithSize("ABCDABCDABCDABCDABCDABCD", comp_buf[0..256]);
const m = decompressWithSize(comp_buf[0..(n as usize)], out_buf[0..256]);
```

## License

MIT
