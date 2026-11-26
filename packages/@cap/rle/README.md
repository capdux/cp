# @cap/rle

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rle-decode-fast` ([https://github.com/okaneco/rle-decode-fast](https://github.com/okaneco/rle-decode-fast))（原库名 `rle-decode-fast` → CP 包名 `@cap/rle`）

零堆分配的 CP 轻量级字节游程压缩（Apple/TIFF PackBits RLE）与时序整数差值编码（Prometheus/Gorilla Delta & Delta-of-Delta）引擎 — 洁净室 MIT 实现。

Zero-allocation Apple/TIFF PackBits byte run-length compression and Prometheus/Gorilla-style Delta & Delta-of-Delta time-series integer encoding for CP.

## 安装 / Install

```bash
cap add @cap/rle
```

## API 参考 / API Reference

| 函数 / Function | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `encodePackBits` | `(input: string, out: []u8) -> string?` | Apple/TIFF PackBits 字节游程压缩 |
| `decodePackBits` | `(encoded: string, out: []u8) -> string?` | PackBits 解压（支持 128 NOP，严格防截断/越界） |
| `encodeDeltaI64` / `decodeDeltaI64` | `([]const i64, []i64) -> usize` | 一阶差分编解码 |
| `encodeDeltaOfDeltaI64` / `decodeDeltaOfDeltaI64` | `([]const i64, []i64) -> usize` | 二阶差分（等间隔时间戳全为 `0`） |

## 使用示例 / Usage

```cp
import { encodePackBits, decodePackBits, encodeDeltaOfDeltaI64, decodeDeltaOfDeltaI64 } from "@cap/rle";

let enc_buf: [128]u8 = undefined;
const compressed = encodePackBits("AAAAAAAAAABBBBB", enc_buf[0..128]).?; // 4 bytes!

let ts: [4]i64 = [1000, 1060, 1120, 1180];
let dod: [4]i64 = undefined;
* = encodeDeltaOfDeltaI64(ts[0..4], dod[0..4]); // [1000, 60, 0, 0]
```

## License

MIT
