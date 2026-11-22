# `cap:fasta` — 纯 CP 生物信息学 FASTA DNA/RNA/蛋白质序列零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:science`，导入请使用 `import { ... } from "cap:science"`。

## 概述

`cap:fasta` 提供纯 CP、零外部依赖、零堆分配的 NCBI / EMBL FASTA 生物序列流式解析与核酸工具：

- **格式判定与多记录流式迭代**：`isFasta` 与 `FastaIter` 自动跳过 UTF-8 BOM、前导空行与 `;` 注释行，兼容 `\r\n`/`\n`、多行折行序列、无描述头与空序列记录；
- **序列视图与 GC 含量**：`FastaRecord` 零拷贝拆分 `id`、`description` 与 `raw_seq`，提供 `seqLen()`、`gcContent()`（大小写不敏感，支持 RNA `U`）及 `copyCleanSeq(out)`；
- **IUPAC DNA 反向互补**：`dnaComplement` 与 `reverseComplementDna` 完整支持标准碱基与全部 IUPAC 简并碱基（`R/Y/S/W/K/M/B/D/H/V/N`）并保留大小写，零堆分配输出；
- **单遍数据集统计**：`summarizeFasta` 统计 `record_count`, `total_bases`, `min_len`, `max_len`, `gc_bases`, `at_bases`, `n_bases` 及 `overallGc()`。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `isFasta(data): bool` | 跳过前导空行与 `;` 注释行后检测 `'>'` 起始符 |
| `FastaRecord` | 单条序列视图：`id`, `description`, `raw_seq`；附 `seqLen(): usize`, `gcContent(): f64`, `copyCleanSeq(out): usize?` |
| `FastaIter` | 零分配多记录流式迭代器：`init(data)`、`next(*_): FastaRecord?` |
| `dnaComplement(base): u8` | 标准 + IUPAC 简并碱基互补映射（保留大小写） |
| `reverseComplementDna(seq, out): usize?` | 零堆分配计算 DNA 反向互补序列（自动忽略空白/换行，溢出返回 `null`） |
| `FastaStats` / `summarizeFasta(data): FastaStats` | 单遍统计数据集长度分布与 `overallGc(): f64` |

## 示例

```cp
import std from "std";
import { FastaIter, reverseComplementDna, summarizeFasta } from "cap:science";

fn analyzeFasta(fasta_text: []const u8): f64 {
    let it = FastaIter.init(fasta_text);
    let rc_buf: [256]u8 = std.mem.zeroes([256]u8);
    if (rec := it.next()) {
        * = reverseComplementDna(rec.raw_seq, rc_buf[0..]);
    }
    return summarizeFasta(fasta_text).overallGc();
}
```
