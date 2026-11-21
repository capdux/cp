# `cap:ssz` — 纯 CP Ethereum SimpleSerialize (SSZ) 零分配编解码与校验库

## 概述

`cap:ssz` 提供纯 CP、零外部依赖、零堆分配的 Ethereum Consensus SimpleSerialize（SSZ）确定性小端编解码、Bitlist 哨兵位与 Bitvector 尾零校验、变长列表 4B LE Offset 表零拷贝迭代，以及 32 字节 Merkleization Chunk 辅助计算。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `SSZ_OFFSET_BYTES` / `SSZ_CHUNK_BYTES` | 常量：`4`、`32` |
| `SszWriter` | 零分配写入器：`init(buf)`、`written()`、`remaining()`、`writeBool`、`writeU8`/`U16`/`U32`/`U64`/`U128`、`writeOffset`、`writeFixedBytes`、`writeZeroPad`、`writeBitlist`（附哨兵位）、`writeBitvector` |
| `SszReader` | 零拷贝游标：`init(data)`、`remaining()`、`isEof()`、`readBool`（严格仅允 `0`/`1`）、`readU8`/`U16`/`U32`/`U64`/`U128`、`readOffset`、`readFixedBytes(n)` |
| `sszBitlistLen` / `sszBitlistGet` | Bitlist 哨兵位解析与按位读取（严格拒绝空切片与末字节 `0x00`） |
| `sszBitvectorValid` / `sszBitvectorGet` | Bitvector 字节长度与尾部未使用高位全零校验 |
| `SszOffsetListIter` | 变长列表 Offset 表校验与零拷贝迭代：`init(data)`, `isValid()`, `len()`, `next(*_): ([]const u8)?` |
| `sszChunkCount` / `sszNextPowerOfTwo` | 32 字节 Chunk 数量与 Merkle 树叶子 2 的幂计算 |

## 示例

```cp
import { SszWriter, SszReader, sszBitlistLen } from "cap:ssz";

fn encodeHeader(buf: []u8, slot: u64): usize {
    let w = SszWriter.init(buf);
    if (!w.writeU64(slot)) return 0;
    const flags: [3]bool = [true, false, true];
    if (!w.writeBitlist(flags[0..])) return 0;
    return w.written().len;
}
```
