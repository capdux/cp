# `cap:bed` — 纯 CP UCSC BED3–BED12 基因组区间零分配解析与重叠计算库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:science`，导入请使用 `import { ... } from "cap:science"`。

## 概述

`cap:bed` 提供纯 CP、零外部依赖、零堆分配的 UCSC Genome Browser BED3 / BED6 / BED12（0-based 半开区间 `[start, end)`）流式解析与基因组区间重叠计算能力：

- **BED3–BED12 全字段解析**：支持 Tab 与空白分隔，严格校验 `start <= end`、`thick_start <= thick_end` 与数值/链方向合法性；
- **零分配区间几何计算**：提供 `length()`、`overlaps(other)`、`overlapLen(other)` 与 `containsPos(pos)`，严格区分染色体并遵循半开区间语义；
- **UCSC 头部自动跳过**：`BedIter` 自动跳过 UTF-8 BOM、空行、`#` 注释行、`track ...` 与 `browser ...` 头行；
- **单遍数据集统计**：`summarizeBed` 统计记录数、总碱基跨度、长度极值、正负链计数与列数范围。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `BedStrand` | 链方向枚举：`plus='+'`, `minus='-'`, `unknown='.'` |
| `BedRecord` | BED3–BED12 零拷贝记录；附 `length(): u64`, `overlaps(other): bool`, `overlapLen(other): u64`, `containsPos(pos): bool` |
| `parseBedLine(line): BedRecord?` | 解析单行 BED3–BED12 记录（兼容 `\r\n`） |
| `BedIter` | 零分配流式迭代器：`init(data)`、`next(*_): BedRecord?`（附 `had_error: bool`） |
| `BedStats` / `summarizeBed(data): BedStats` | 单遍统计 `record_count`, `total_span_bp`, `min_len`, `max_len`, `plus_count`, `minus_count`, `min_fields`, `max_fields` |

## 示例

```cp
import { BedIter, summarizeBed } from "cap:science";

fn countOverlaps(bed_text: []const u8): u64 {
    let it = BedIter.init(bed_text);
    const first = it.next() ?? return 0;
    let total_ov: u64 = 0;
    while (rec := it.next()) {
        total_ov += first.overlapLen(rec);
    }
    * = summarizeBed(bed_text);
    return total_ov;
}
```
