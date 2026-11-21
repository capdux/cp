# `cap:ttf` — 纯 CP TrueType / OpenType SFNT 零分配字体表解析库

## 概述

`cap:ttf` 提供纯 CP、零外部依赖、零堆分配的 TrueType / OpenType SFNT（`.ttf` / `.otf`）二进制字体容器解析器，支持 SFNT 12 字节偏移表解码、表目录流式迭代、按 4 字节标签查找表记录、带边界溢出防护的切片视图、`head`/`maxp` 元数据提取与 4 字节对齐大端无符号回绕校验和计算。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `SfntFlavor` | SFNT 容器风格枚举：`truetype` (`0x00010000`), `cff` (`"OTTO"`), `apple_true` (`"true"`), `unknown` |
| `SfntOffsetTable` | 12 字节大端偏移表：`flavor`, `sfnt_version`, `num_tables`, `search_range`, `entry_selector`, `range_shift` |
| `SfntTableRecord` | 16 字节大端表目录记录：`tag: [4]u8`, `checksum`, `offset`, `length`；方法：`slice(data)`, `tagEquals(other)` |
| `TtfHeadMetrics` | `head` 表度量：`units_per_em`, `x_min`, `y_min`, `x_max`, `y_max`, `index_to_loc_format` |
| `parseSfntHeader(data)` | 解析 SFNT 头部并校验表目录长度不越界 |
| `SfntTableIter` | 表目录迭代器：`init(data)`、`next(*_): SfntTableRecord?` |
| `findSfntTable(data, tag)` | 按 4 字节 tag 查找表记录 |
| `parseTtfHead(data)` | 解析 `head` 表并严格校验 `magicNumber == 0x5F0F3CF5` |
| `parseTtfNumGlyphs(data)` | 从 `maxp` 表提取字形总数 `numGlyphs` |
| `calcSfntChecksum(table_bytes)` | 计算大端 4 字节对齐无符号回绕校验和 |

## 示例

```cp
import { parseSfntHeader, parseTtfHead, parseTtfNumGlyphs } from "cap:ttf";

fn inspectFont(font_bytes: []const u8): bool {
    if (parseSfntHeader(font_bytes) == null) return false;
    if (head := parseTtfHead(font_bytes)) {
        if (glyphs := parseTtfNumGlyphs(font_bytes)) {
            return head.units_per_em > 0 && glyphs > 0;
        }
    }
    return false;
}
```
