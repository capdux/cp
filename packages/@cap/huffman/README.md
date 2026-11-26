# @cap/huffman

零堆分配的 CP RFC 1951 规范霍夫曼（Canonical Huffman）前缀编码器与解码器 — 洁净室 MIT 实现，对标 Rust `huffman-compress` 与 RFC 1951 §3.2.2，支持全 256 符号表与 16 位限长。

Zero-allocation RFC 1951 Canonical Huffman prefix encoder & decoder for CP — clean-room MIT parity with Rust `huffman-compress` and RFC 1951.

## 安装 / Install

```bash
cap add @cap/huffman
```

## API 参考 / API Reference

- **`HuffmanCodec`**（256 符号表，最大码长 `MAX_CODE_BITS = 16`）：
  - `fromFrequencies(freqs: []const u32) -> HuffmanCodec`（从频次数组构建限长规范霍夫曼码表）
  - `fromCodeLengths(lengths: []const u8) -> HuffmanCodec`（RFC 1951 仅凭码长重建规范码表）
  - `codeLength(sym: u8) -> u8`、`codeBits(sym: u8) -> u16`
  - `encode(src: []const u8, dst: []u8) -> i64`（4 字节大端原长度头 + MSB-first 位流，错误返回 `-1`）
  - `decode(src: []const u8, dst: []u8) -> i64`（精确按原长度解码，畸形/截断/缓冲不足返回 `-1`）

## 使用示例 / Usage

```cp
import { HuffmanCodec } from "@cap/huffman";

let freqs: [256]u32 = undefined;
// ... populate freqs from input ...
const codec = HuffmanCodec.fromFrequencies(freqs[0..256]);
const enc_len = codec.encode(src, enc_buf);
const dec_len = codec.decode(enc_buf[0..(enc_len as usize)], dec_buf);
```

## License

MIT
