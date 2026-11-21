# `cap:dex` — 纯 CP Android Dalvik Executable (DEX) 零分配解析库

## 概述

`cap:dex` 提供纯 CP、零外部依赖、零堆分配的 Android DEX 二进制文件头（112 字节）、LE/BE 端序识别、ULEB128 变长整数解码、MUTF-8 字符串表零拷贝索引读取与 `map_list` 区域表迭代器，严格防越界与整数溢出。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `DEX_HEADER_SIZE` / `DEX_ENDIAN_CONSTANT` / `DEX_REVERSE_ENDIAN_CONSTANT` | 常量：`112` (`0x70`)、`0x12345678`、`0x78563412` |
| `DEX_TYPE_*` | 常用 `map_item` 类型码常量（`HEADER_ITEM`, `STRING_ID_ITEM`, `TYPE_ID_ITEM`, `MAP_LIST`, `STRING_DATA_ITEM` 等） |
| `isDex(data)` | 校验 8 字节魔数 `"dex\n"` + 3 位 ASCII 数字版本 + `'\0'` |
| `DexHeader` / `parseDexHeader(data)` | 解析完整 112 字节头，自动识别 LE/BE 端序，严格校验 `header_size == 112`、`file_size <= data.len` 及全部段边界防溢出 |
| `Uleb128Result` / `readUleb128(data, pos)` | 1–5 字节 ULEB128 无符号整数解码（防 `u32` 溢出与截断） |
| `readDexString(data, header, idx)` | 通过 `string_ids_off + idx * 4` 定位 `string_data_off`，跳过 ULEB128 `utf16_size` 返回零拷贝 `\0` 结尾字符串切片 |
| `DexMapItem` / `DexMapIter` | 解析 `map_off` 处 4 字节 `size` 与 12 字节条目表（`item_type`, `size`, `offset`） |

## 示例

```cp
import { isDex, parseDexHeader, readDexString } from "cap:dex";

fn firstDexString(bytes: []const u8): ([]const u8)? {
    if (!isDex(bytes)) return null;
    const hdr = parseDexHeader(bytes) ?? return null;
    return readDexString(bytes, hdr, 0);
}
```
