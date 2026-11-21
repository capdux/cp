# `cap:gff` — 纯 CP GFF3 / GTF2 基因特征注释零分配解析库

## 概述

`cap:gff` 提供纯 CP、零外部依赖、零堆分配的 GFF3（`##gff-version 3`, `key=val;`）与 GTF2/GFF2（`key "val";`）基因特征注释解析器（1-based 闭区间 `[start, end]`）：

- **严格 9 列解析**：校验 Tab 分隔 9 列、`start >= 1 && start <= end`、可选浮点分值（`.` 缺失）、链方向（`+`/`-`/`.`/`?`）与相位（`0`/`1`/`2`/`255`）；
- **双语法零拷贝属性查找**：`GffRecord.findAttr(key)` 同时兼容 GFF3 `ID=gene1;Parent=chr1` 与 GTF2 `gene_id "ENSG...";`（自动处理引号与引号内分号）；
- **`##FASTA` 安全截断**：`GffIter` 自动跳过空行、`#`/`##` 注释与指令行，遇 `##FASTA` 内嵌序列区立即停止并置位 `reached_fasta = true`；
- **单遍特征统计**：`summarizeGff` 统计 `record_count`, `gene_count`, `exon_count`, `cds_count`, `total_feature_bp`, `plus_count`, `minus_count`。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `GFF_PHASE_NONE` (`255`) | 相位缺失 (`"."`) 常量 |
| `isGff3(data): bool` | 校验首行以 `"##gff-version 3"` 开头 |
| `GffRecord` | 9 列基因特征零拷贝记录；附 `length(): u64` (`end - start + 1`) 与 `findAttr(key): ([]const u8)?` |
| `parseGffLine(line): GffRecord?` | 解析单行 9 列 Tab 分隔 GFF3/GTF2 记录 |
| `GffIter` | 零分配流式迭代器：`init(data)`、`next(*_): GffRecord?`（附 `reached_fasta`, `had_error`） |
| `GffStats` / `summarizeGff(data): GffStats` | 单遍特征统计汇总 |

## 示例

```cp
import { isGff3, GffIter, summarizeGff } from "cap:gff";

fn inspectGff(text: []const u8): usize {
    * = isGff3(text);
    let it = GffIter.init(text);
    while (rec := it.next()) {
        * = rec.findAttr("ID");
    }
    return summarizeGff(text).gene_count;
}
```
