# `cap:pcf` — 零分配 X11 Portable Compiled Format (`.pcf`) 点阵字体解析库

> **模块路径**：`lib/pcf/`（入口 `lib/pcf/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 X11 PCF 二进制规范（TOC 恒为小端，各表内部按表级 `format` 标志动态切换小端/大端），支持 TOC 迭代、5B 压缩与 12B 未压缩字形度量解码、1D/2D `PCF_BDF_ENCODINGS` O(1) 字形索引查表以及 4B 对齐的 `PCF_PROPERTIES` 属性迭代。

---

## 一、导入方式

```cp
import {
    PCF_MAGIC,
    PCF_TOC_ENTRY_SIZE,
    PCF_PROPERTIES,
    PCF_ACCELERATORS,
    PCF_METRICS,
    PCF_BITMAPS,
    PCF_INK_METRICS,
    PCF_BDF_ENCODINGS,
    PCF_SWIDTHS,
    PCF_GLYPH_NAMES,
    PCF_BDF_ACCELERATORS,
    PCF_DEFAULT_FORMAT,
    PCF_COMPRESSED_METRICS,
    PCF_BYTE_MASK,
    PCF_BIT_MASK,
    PCF_GLYPH_PAD_MASK,
    PcfTocEntry,
    PcfMetrics,
    PcfEncodingsHeader,
    PcfProperty,
    PcfTocIter,
    PcfPropIter,
    isPcf,
    findPcfTable,
    parsePcfMetricsCount,
    readPcfMetric,
    parsePcfEncodings,
    lookupPcfGlyphIndex,
} from "cap:pcf";
```

---

## 二、核心数据结构与常量

| 符号 | 字段 / 值 | 说明 |
| :--- | :--- | :--- |
| `PCF_MAGIC` | `"\x01fcp"` | 4 字节 PCF 文件魔数 |
| `PCF_BYTE_MASK` | `0x00000004` | 表内字节序标志：置位表示 Big-Endian，清零表示 Little-Endian |
| `PCF_COMPRESSED_METRICS` | `0x00000100` | 置位表示 5B 压缩度量（每字节减去 `0x80`） |
| `PcfTocEntry` | `table_type`, `format`, `size`, `offset`, `payload` | 零拷贝 TOC 表项 |
| `PcfMetrics` | `left_side_bearing`, `right_side_bearing`, `character_width`, `character_ascent`, `character_descent`, `character_attributes` | 统一解包后的字形度量 |
| `PcfEncodingsHeader` | `min_char_or_byte2`, `max_char_or_byte2`, `min_byte1`, `max_byte1`, `default_char` | 1D/2D 编码矩阵头 |
| `PcfProperty` | `name`, `is_string`, `str_value`, `int_value` | `PCF_PROPERTIES` 字符串或整型属性 |

---

## 三、核心 API

- `isPcf(data)` / `PcfTocIter.init(data)` / `findPcfTable(data, table_type)` — 校验魔数与 TOC 边界并零拷贝定位表切片。
- `parsePcfMetricsCount(table_payload)` / `readPcfMetric(table_payload, idx)` — 自动适配压缩 5B 与未压缩 12B LE/BE 度量。
- `parsePcfEncodings(table_payload)` / `lookupPcfGlyphIndex(table_payload, codepoint)` — O(1) 编码矩阵查找，遇到 `0xFFFF` 缺失字形哨兵返回 `null`。
- `PcfPropIter.init(table_payload)` / `next(*_)` — 自动处理 9B 属性记录后的 4B 填充并从字符串池零拷贝提取属性名与值。

---

## 四、使用示例

```cp
import {
    PCF_METRICS,
    PCF_BDF_ENCODINGS,
    findPcfTable,
    lookupPcfGlyphIndex,
    readPcfMetric,
} from "cap:pcf";

fn inspectPcfGlyph(pcf_bytes: []const u8, codepoint: u32) {
    if (enc_tbl := findPcfTable(pcf_bytes, PCF_BDF_ENCODINGS)) {
        if (glyph_idx := lookupPcfGlyphIndex(enc_tbl.payload, codepoint)) {
            if (met_tbl := findPcfTable(pcf_bytes, PCF_METRICS)) {
                if (m := readPcfMetric(met_tbl.payload, glyph_idx as u32)) {
                    * = m.character_width;
                }
            }
        }
    }
}
```
