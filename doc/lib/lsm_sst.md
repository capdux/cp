# `cap:lsm_sst` — LSM-Tree Sorted String Table (SSTable) 标准库

## 概述

`cap:lsm_sst`（同时支持 `cap.lsm_sst` 挂载）提供工业级纯 CP 实现的 LSM-Tree Sorted String Table (SSTable) 存储格式与查询引擎，包含：
- **前缀压缩数据块与重启点（Restart Points）**：RocksDB/LevelDB 风格前缀共享编码，支持 `O(log R)` 重启点二分查找与线性扫描。
- **Kirsch-Mitzenmacher 双哈希布隆过滤器**：基于 `Wyhash` 双哈希的紧凑型过滤器，零假阴性、低假阳性率。
- **SSTable 完整容器格式**：自动切分 Data Block、生成 Index Block 与 Bloom Filter Block，所有块均附带 5 字节 Trailer（压缩类型 + CRC32C 校验和）与 48 字节定长 Footer（含 8 字节魔数 `"CPSST001"`）。
- **多路去重归并迭代器 (`SstMergeIter`)**：按从新到旧优先级对多个 SSTable 进行有序归并，自动处理新值覆盖与墓碑（Tombstone）删除语义。

## 核心类型

| 类型 / 常量 | 所在模块 | 说明 |
| --- | --- | --- |
| `SstError` | `block.cp` | 错误集合：`UnsortedKeys`、`CorruptedChecksum`、`InvalidFooter`、`InvalidBlock`、`BufferTooSmall`、`OutOfMemory` |
| `BlockHandle` | `block.cp` | 16 字节定长块定位句柄（`offset: u64`, `size: u64`） |
| `BlockBuilder` / `BlockReader` / `BlockIter` | `block.cp` | 前缀压缩数据块构建器、只读解析器与二分定位迭代器 |
| `BloomFilter` | `bloom.cp` | 双哈希布隆过滤器构建与成员探测 |
| `SstOptions` / `SstGetResult` / `SstOwnedEntry` | `sstable.cp` | SSTable 构建配置、点查结果与范围扫描结果条目 |
| `SstBuilder` / `SstReader` | `sstable.cp` | SSTable 顺序写入构建器与校验型只读查询器 |
| `SstMergeIter` | `sstable.cp` | 多 SSTable 从新到旧去重及墓碑过滤归并迭代器 |

## API 速查

- **数据块**：`BlockBuilder.init(alloc, restart_interval)`、`add(key, value)`、`finish()`、`reset()`；`BlockReader.init(data)`、`iter(alloc)`；`BlockIter.seekToFirst()`、`seek(target)`、`next()`、`key()`、`value()`。
- **布隆过滤器**：`buildBloomFilter(alloc, keys, bits_per_key)`、`mayContain(filter_bytes, key)`（或 `BloomFilter.build` / `BloomFilter.mayContain`）。
- **SSTable 构建与读取**：
  - `SstBuilder.init(alloc, opts)`、`put(key, value)`、`delete(key)`、`finish()`、`deinit()`
  - `SstReader.init(alloc, data)`、`get(key)`、`rangeScan(alloc, start, end)`、`deinit()`
- **多路归并**：`SstMergeIter.init(alloc, readers)`、`seekToFirst()`、`seek(target)`、`next()`、`isValid()`、`key()`、`value()`、`deinit()`。

## 示例

```cp
import { SstBuilder, SstReader, SstOptions } from "cap:lsm_sst";

let builder = SstBuilder.init(alloc, SstOptions{ .block_size = 4096 });
defer builder.deinit();
try builder.put("alpha", "100");
try builder.put("beta", "200");
const sst_bytes = try builder.finish();
defer alloc.free(sst_bytes);

let reader = try SstReader.init(alloc, sst_bytes);
defer reader.deinit();
if (try reader.get("alpha")) |res| {
    // res.value == "100", res.is_tombstone == false
}
```
