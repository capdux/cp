# `cap:rar` — RAR4 & RAR5 归档容器格式零分配解析库

纯 CP、零堆分配的 RAR4 与 RAR5 归档容器解析库，支持 RAR4/RAR5 魔数识别、IEEE 802.3 CRC32 校验、RAR5 7-bit Vint (1..10B) 编解码、RAR5 块流迭代（含 `extra_area` / `data_area` 与 `Rar5FileEntry` 解析）以及 RAR4 块流低 16 位 CRC32 校验迭代。

## 导入

```cp
import {
    RAR4_SIGNATURE,
    RAR5_SIGNATURE,
    RAR_VER_4,
    RAR_VER_5,
    RAR5_HEAD_MAIN,
    RAR5_HEAD_FILE,
    RAR5_HEAD_SERVICE,
    RAR5_HEAD_CRYPT,
    RAR5_HEAD_ENDARC,
    HFL_EXTRA,
    HFL_DATA,
    HFL_SKIP_IF_UNKNOWN,
    HFL_SPLIT_BEFORE,
    HFL_SPLIT_AFTER,
    FILE_FLAG_DIR,
    FILE_FLAG_UTIME,
    FILE_FLAG_CRC32,
    FILE_FLAG_UNP_UNKNOWN,
    RAR4_TYPE_MAIN,
    RAR4_TYPE_FILE,
    RAR4_TYPE_ENDARC,
    detectRarVersion,
    isRar,
    rarCrc32,
    Rar5Vint,
    decodeRar5Vint,
    encodeRar5Vint,
    Rar5Block,
    Rar5BlockIter,
    iterRar5Blocks,
    Rar5FileEntry,
    parseRar5FileEntry,
    Rar4Block,
    Rar4BlockIter,
    iterRar4Blocks,
} from "cap:rar";
```

## 核心 API

- **魔数与校验**：
  - `detectRarVersion(data: []const u8): u8?`（返回 `4`、`5` 或 `null`）、`isRar(data: []const u8): bool`
  - `rarCrc32(data: []const u8): u32`（IEEE 802.3 `0xEDB88320`）
  - `decodeRar5Vint(data: []const u8): Rar5Vint?`、`encodeRar5Vint(val: u64, out: []u8): usize?`
- **RAR5 块与文件条目**：
  - `iterRar5Blocks(data: []const u8): Rar5BlockIter` — 校验 8B 签名与逐块 CRC32，分离 `body`/`extra_area`/`data_area`，遇 `RAR5_HEAD_ENDARC` 停止。
  - `parseRar5FileEntry(block: Rar5Block): Rar5FileEntry?` — 附 `isDir()`, `hasMtime()`, `hasCrc32()`, `compMethod()`, `isSolid()`。
- **RAR4 块流**：
  - `iterRar4Blocks(data: []const u8): Rar4BlockIter` — 校验 7B 签名与逐块低 16 位 CRC32，遇 `RAR4_TYPE_ENDARC` 停止。

## 示例

```cp
import { iterRar5Blocks, parseRar5FileEntry, RAR5_HEAD_FILE } from "cap:rar";

fn listRar5(archive: []const u8) {
    let it = iterRar5Blocks(archive);
    while (true) {
        if (blk := it.next()) {
            if (blk.header_type == RAR5_HEAD_FILE) {
                if (f := parseRar5FileEntry(blk)) {
                    * = f.name;
                    * = f.data;
                }
            }
        } else {
            break;
        }
    }
}
```
