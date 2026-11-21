# `cap:lzh` — LZH / LHA `.lzh` 归档格式零分配解析库

纯 CP、零堆分配的 LZH / LHA 归档格式解析库，完整支持 Level 0 / Level 1 / Level 2 报头、Level 0/1 头部字节累加校验和、Level 2 `EXT_COMMON_CRC` 报头校验、CRC-16/ARC (`0xA001`) 数据校验、扩展头链零拷贝迭代以及多条目归档顺序解析。

## 导入

```cp
import {
    LZH_METHOD_LH0,
    LZH_METHOD_LH4,
    LZH_METHOD_LH5,
    LZH_METHOD_LH6,
    LZH_METHOD_LH7,
    LZH_METHOD_LHD,
    LEVEL_0,
    LEVEL_1,
    LEVEL_2,
    EXT_COMMON_CRC,
    EXT_FILENAME,
    EXT_DIRECTORY,
    EXT_UNIX_PERM,
    EXT_UNIX_GID_UID,
    EXT_UNIX_MTIME,
    lzhHeaderSum,
    lzhCrc16,
    isLzhMethod,
    isLzh,
    LzhExtHeader,
    LzhExtIter,
    LzhEntry,
    parseLzhEntry,
    LzhEntryIter,
    iterLzhEntries,
} from "cap:lzh";
```

## 核心 API

- **校验和与格式嗅探**：
  - `lzhHeaderSum(bytes: []const u8): u8` — Level 0/1 头部字节累加 mod 256。
  - `lzhCrc16(data: []const u8): u16` — CRC-16/ARC（反射多项式 `0xA001`，初值 `0x0000`，无最终异或）。
  - `isLzhMethod(m: []const u8): bool`、`isLzh(data: []const u8): bool`。
- **条目与扩展头解析**：
  - `parseLzhEntry(data: []const u8): LzhEntry?` — 完整支持 Level 0 / 1 / 2，自动提取 `name`/`dir_name`/`ext_headers_bytes`/`data`，附 `isStored()`, `isDir()`, `extHeaders()`, `verifyDataCrc()`。
  - `LzhExtIter` — 按 2B `next_size` 零拷贝链式迭代扩展头，遇 `0` 终止。
  - `iterLzhEntries(data: []const u8): LzhEntryIter` — 顺序迭代归档条目直至 `0x00` 哨兵或 EOF，畸形置 `hasError()`。

## 示例

```cp
import { iterLzhEntries } from "cap:lzh";

fn listLzh(archive: []const u8) {
    let it = iterLzhEntries(archive);
    while (true) {
        if (ent := it.next()) {
            * = ent.name;
            if (ent.isStored()) {
                * = ent.verifyDataCrc();
            }
        } else {
            break;
        }
    }
}
```
