# @cap/snappy-block

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:snap` ([https://github.com/BurntSushi/rust-snappy](https://github.com/BurntSushi/rust-snappy))（原库名 `snap` → CP 包名 `@cap/snappy-block`）

零堆分配的 CP Google Snappy Raw Block 极速无损块压缩与解压引擎 — 洁净室 MIT 实现，对标 Rust `snap` crate，100% 兼容 Parquet / Kafka / LevelDB / RocksDB / gRPC 标准 Snappy 字节流格式。

Zero-allocation Google Snappy Raw Block lossless compressor & decompressor for CP — clean-room MIT parity with Rust `snap`.

## 安装 / Install

```bash
cap add @cap/snappy-block
```

## API 参考 / API Reference

| 函数 / Function | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `maxCompressedLen` | `(input_len: usize) -> usize` | 计算最坏情况压缩缓冲区大小 `32 + len + len / 6` |
| `uncompressedLen` | `(src: []const u8) -> i64` | 仅解析头部 varint32 原始长度（非法返回 `-1`） |
| `compress` | `(src: []const u8, dst: []u8) -> i64` | 压缩为标准 Snappy Raw Block（缓冲不足返回 `-1`） |
| `decompress` | `(src: []const u8, dst: []u8) -> i64` | 解压并支持 Literal / Copy-1 / Copy-2 / Copy-4 与重叠 RLE |

## 使用示例 / Usage

```cp
import { compress, decompress, uncompressedLen } from "@cap/snappy-block";

let comp: [256]u8 = undefined;
let out: [256]u8 = undefined;
const n = compress("Snappy Snappy Snappy Snappy!", comp[0..256]);
const orig = uncompressedLen(comp[0..(n as usize)]); // 28
const m = decompress(comp[0..(n as usize)], out[0..256]);
```

## License

MIT
