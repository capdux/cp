# `@cap/crc32c` — 零分配 CRC32C（Castagnoli）校验和、流式哈希器与并行分片合并

> 独立 Clean-Room 纯 CP 实现，遵循 RFC 3720 / Castagnoli（`0x82F63B78`）公开标准（对标 Rust `crc32c` crate 与 npm `fast-crc32c`）| 许可协议：MIT

## 概述

`@cap/crc32c` 提供纯 CP、零外部依赖、零堆分配的 CRC32C（Castagnoli）完整实现，广泛用于 gRPC、LevelDB、RocksDB、Kafka、Google Cloud Storage、Amazon S3、iSCSI 与 TensorFlow TFRecord：
- **标准查表计算**：`crc32c` / `crc32cAppend` 与流式预建表 `Crc32cHasher`；
- **O(log N) 并行分片合并**：`crc32cCombine(crc1, crc2, len2)` 基于 GF(2) 矩阵平方，无需重读数据即可合并多核/分块校验和；
- **LevelDB / RocksDB / TFRecord 屏蔽算法**：`mask(crc)` / `unmask(masked)` 避免嵌套校验和退化。

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `CASTAGNOLI_POLY` (`0x82F63B78`), `MASK_DELTA` (`0xa282ead8`) | Castagnoli 多项式与 LevelDB 屏蔽常量 |
| `crc32c(data: []const u8): u32` | 一次性计算 CRC32C 校验和 |
| `crc32cAppend(prev: u32, data: []const u8): u32` | 在已有 CRC32C 基础上增量追加数据 |
| `crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32` | O(log len2) 合并两段独立数据的 CRC32C |
| `mask(crc: u32): u32` / `unmask(masked: u32): u32` | LevelDB / RocksDB / TFRecord 标准屏蔽与还原 |
| `Crc32cHasher` | 流式哈希器：`init()`、`update(data)`、`finalize()`、`finalizeMasked()`、`bytesHashed()`、`reset()` |

## 使用示例

### 示例 1：一次性计算与流式分块计算

```cp
import { crc32c, Crc32cHasher } from "@cap/crc32c";

fn demoBasic() {
    const check = crc32c("123456789"); // 0xE3069283 (RFC 3720 标准向量)

    let h = Crc32cHasher.init();
    h.update("1234");
    h.update("56789");
    const streamed = h.finalize(); // == check
    * = check;
    * = streamed;
}
```

### 示例 2：多核分片并行校验合并（`crc32cCombine`）与 TFRecord 屏蔽

```cp
import { crc32c, crc32cCombine, mask, unmask } from "@cap/crc32c";

fn demoCombineAndMask() {
    const c1 = crc32c("hello ");
    const c2 = crc32c("world!");
    const combined = crc32cCombine(c1, c2, 6); // == crc32c("hello world!")

    const m = mask(combined);
    const orig = unmask(m); // == combined
    * = orig;
}
```
