# @cap/crc32

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:simd-adler32` ([https://github.com/mcountryman/simd-adler32](https://github.com/mcountryman/simd-adler32)) / `crates.io:crc-catalog` ([https://github.com/akhilles/crc-catalog](https://github.com/akhilles/crc-catalog))（原库名 `simd-adler32` → CP 包名 `@cap/crc32`）

Zero-allocation IEEE 802.3 CRC-32, Castagnoli CRC-32C, Adler-32, and O(log N) CRC-32 combine engine for CP — clean-room MIT parity with Rust `crc32fast` (918M downloads on crates.io).

纯 CP 零堆分配高速校验和引擎，完整覆盖 IEEE CRC-32（ZIP/GZIP/PNG/Ethernet）、Castagnoli CRC-32C（RocksDB/gRPC/S3/iSCSI/EXT4）、Adler-32（RFC 1950）与多线程分块 O(log N) 增量合并。

## Install / 安装

```bash
cap add @cap/crc32
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Crc32Hasher` | `init() / update(*_, data) / finalize() -> u32 / reset(*_)` | Streaming IEEE 802.3 CRC-32 (`0xEDB88320`) |
| `Crc32cHasher` | `init() / update(*_, data) / finalize() -> u32 / reset(*_)` | Streaming Castagnoli CRC-32C (`0x82F63B78`) |
| `crc32Ieee` | `(data: string) -> u32` | One-shot IEEE CRC-32 checksum |
| `crc32c` | `(data: string) -> u32` | One-shot Castagnoli CRC-32C checksum |
| `adler32` | `(data: string) -> u32` | RFC 1950 Adler-32 checksum |
| `combineCrc32Ieee` | `(crc1: u32, crc2: u32, len2: usize) -> u32` | O(log N) GF(2) matrix combine of two chunk CRC-32 values |

## Usage / 使用示例

```cp
import { Crc32Hasher, crc32Ieee, crc32c, adler32, combineCrc32Ieee } from "@cap/crc32";

const ieee = crc32Ieee("123456789");   // 0xCBF43926
const c32c = crc32c("123456789");      // 0xE3069283
const adl  = adler32("Wikipedia");     // 0x11E60398

const c1 = crc32Ieee("1234");
const c2 = crc32Ieee("56789");
const merged = combineCrc32Ieee(c1, c2, 5); // 0xCBF43926
```
