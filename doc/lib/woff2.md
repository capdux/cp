# `cap:woff2` — 纯 CP W3C WOFF2 网页字体二进制头与表目录零分配解析库

## 概述

`cap:woff2` 提供纯 CP、零外部依赖、零堆分配的 W3C WOFF2 网页字体 48 字节大端文件头解析、`UIntBase128` 变长整数解码（严格遵循 W3C WOFF2 §4.1）及表目录流式迭代器（支持全部 63 个已知表 tag、自定义 4 字节 tag 与 `glyf`/`loca`/`hmtx` 变换长度语义），适用于 CapSurf 与 Bun Web 字体资产预检。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `WOFF2_HEADER_SIZE` / `WOFF2_SIGNATURE` | 常量：`48`、`0x774F4632` (`"wOF2"`) |
| `WOFF2_FLAVOR_TRUETYPE` / `WOFF2_FLAVOR_CFF` / `WOFF2_FLAVOR_COLLECTION` | 字体类型魔数常量：`0x00010000`、`"OTTO"`、`"ttcf"` |
| `Woff2Header` | 完整 48 字节大端头结构，附只读方法 `isTrueType()`, `isCff()`, `isCollection()`, `hasMetadata()` |
| `UIntBase128Result` | Base128 解码结果：`value: u32`, `bytes_read: usize` |
| `decodeUIntBase128(data)` | W3C WOFF2 §4.1 变长整数解码：最多 5 字节、禁止前导 `0x80`、防 `u32` 溢出 |
| `Woff2TableEntry` | 表目录项：`tag: [4]u8`, `flags: u8`, `transform_version: u8`, `orig_length: u32`, `transform_length: u32?` |
| `isWoff2(data)` | 4 字节魔数快速判定是否为 WOFF2 字体 |
| `parseWoff2Header(data)` | 严格校验 48 字节长度、`"wOF2"` 魔数、`reserved == 0`、`num_tables > 0`、`length <= data.len` 及 metadata/private 区间无越界溢出 |
| `Woff2TableIter` | 表目录流式迭代器：`init(data)`, `next(*_): Woff2TableEntry?`；内置 63 个已知表查表、支持 `flags & 0x3F == 63` 自定义 tag 及 `glyf`/`loca` 变换规则 |

## 示例

```cp
import { isWoff2, parseWoff2Header, Woff2TableIter } from "cap:woff2";

fn inspectFont(bytes: []const u8): bool {
    if (!isWoff2(bytes)) return false;
    if (hdr := parseWoff2Header(bytes)) {
        let it = Woff2TableIter.init(bytes);
        let count: usize = 0;
        while (entry := it.next()) {
            if (entry.orig_length >= 0) count += 1;
        }
        return count == (hdr.num_tables as usize);
    }
    return false;
}
```
