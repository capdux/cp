# `cap:sstable` — 纯 CP LevelDB / RocksDB SSTable 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:db`，导入请使用 `import { ... } from "cap:db"`。

## 概述

`cap:sstable` 提供纯 CP、零外部依赖、零堆分配的 LevelDB / RocksDB SSTable 文件结构解析器，完整支持：
- **48B Footer 解析**：识别 LevelDB（`0xDB4775248B80FB57`）与 RocksDB（`0x88E241B785F4CFF7`）魔数，解码 `metaindex_handle` / `index_handle` 并严格校验块边界；
- **Varint64 & `BlockHandle` 编解码**：提供无符号 LEB128 编解码与防溢出校验；
- **块提取与 CRC32C 掩码**：`extractSstBlock` 零拷贝返回负载、压缩算法枚举（`none`/`snappy`/`zlib`/`bzip2`/`lz4`/`lz4hc`/`zstd`）与 `masked_crc32c`，附 `maskCrc` / `unmaskCrc`；
- **前缀压缩 Block 迭代**：`parseSstBlockInfo` 解码末尾 Restart Point 数组，`SstBlockIter` 零分配逐条迭代 `shared_bytes` / `non_shared_bytes` / `key_delta` / `value`。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `SST_FOOTER_SIZE`, `SST_LEVELDB_MAGIC`, `SST_ROCKSDB_MAGIC`, `SST_BLOCK_TRAILER_SIZE` | Footer 大小、魔数与 5B 块尾常量 |
| `SstCompression` | 压缩算法枚举：`none`, `snappy`, `zlib`, `bzip2`, `lz4`, `lz4hc`, `zstd`, `unknown` |
| `BlockHandle`, `decodeVarint64`, `encodeVarint64`, `decodeBlockHandle`, `encodeBlockHandle` | Varint64 与 `BlockHandle` 往返编解码 |
| `SstFooter`, `parseSstFooter(data)`, `isSst(data)` | 48B Footer 解析与探测 |
| `SstBlockView`, `extractSstBlock(file_data, handle)`, `maskCrc`, `unmaskCrc` | 块零拷贝提取与 CRC32C 掩码/解掩码 |
| `SstBlockHeader`, `parseSstBlockInfo(payload)`, `SstRawEntry`, `SstBlockIter` | Restart Point 解析与前缀压缩条目流式迭代 |

## 示例

```cp
import { parseSstFooter, extractSstBlock, parseSstBlockInfo } from "cap:db";

fn countIndexEntries(sst_file: []const u8): usize {
    if (footer := parseSstFooter(sst_file)) {
        if (blk := extractSstBlock(sst_file, footer.index_handle)) {
            if (info := parseSstBlockInfo(blk.payload)) {
                let it = info.entries();
                let n: usize = 0;
                while (e := it.next()) {
                    * = e.key_delta.len;
                    n += 1;
                }
                return n;
            }
        }
    }
    return 0;
}
```
