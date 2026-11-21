# `cap:fits` — 纯 CP 零分配 NASA FITS 2880B HDU 与 80B Card 解析库

## 概述

`cap:fits` 提供纯 CP、零外部依赖、零堆分配的 NASA FITS（Flexible Image Transport System）科学图像与数据立方体 Primary HDU 解析器、80 字节 Keyword Card 迭代器以及类型辅助函数。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `FITS_BLOCK_SIZE` / `FITS_CARD_SIZE` / `FITS_CARDS_PER_BLOCK` | 规范常量：`2880` / `80` / `36` |
| `isFits(data)` | 校验长度 `>= 2880` 且首张 Card 以 `"SIMPLE  ="` 开头 |
| `FitsCard` | 80B 卡片：`keyword`（8B 去尾空格）、`has_value: bool`、`value`（剥离引号外 `/` 注释）、`comment: ([]const u8)?` |
| `FitsCardIter` | 80B Card 迭代器：`init(data)`, `next(*_): FitsCard?`（遇 `"END"` 卡片自动停止） |
| `FitsHeader` | Primary HDU 头摘要：`simple`, `bitpix`（8/16/32/64/-32/-64）, `naxis`, `naxis1..naxis3`, `header_bytes`（2880B 对齐）, `data_bytes`, `card_count` |
| `parseFitsHeader(data)` | 完整解析 Primary HDU 并校验 `SIMPLE`、`BITPIX`、`NAXIS`、`END` 及 2880B 块对齐 |
| `fitsBytesPerPixel(bitpix)` | 返回每像素字节数（`8->1, 16->2, 32/-32->4, 64/-64->8`） |
| `findFitsCard(data, keyword)` | 在头中按关键字查找首张匹配 Card |
| `parseFitsInt(val)` / `parseFitsBool(val)` | 解析 FITS 有符号十进制整数与 `T`/`F` 布尔值 |

## 示例

```cp
import { isFits, parseFitsHeader, findFitsCard } from "cap:fits";

fn inspectObservation(fits_bytes: []const u8): u64? {
    if (!isFits(fits_bytes)) return null;
    const hdr = parseFitsHeader(fits_bytes) ?? return null;
    * = findFitsCard(fits_bytes, "TELESCOP");
    return hdr.data_bytes;
}
```
