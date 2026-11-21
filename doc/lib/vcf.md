# `cap:vcf` — 生物信息学 VCF 4.2/4.3 基因变异零分配解析库

> **导入方式**：`import { isVcf, parseVcfHeader, VcfRecordIter, summarizeVcf } from "cap:vcf";`  
> **内存模型**：100% 纯 CP 零堆分配，所有染色体、等位基因、INFO 键值与样本切片均零拷贝指向原始文本缓冲区。

---

## 核心数据结构

- **`VcfHeader`**：`version`, `reference`, `meta_line_count`, `sample_count`, `records_offset`
- **`VcfRecord`**：`chrom`, `pos: u64`, `id`, `ref_allele`, `alt_allele`, `qual: f64`, `has_qual: bool`, `filter`, `info`, `rest_samples`
  - 辅助方法：`isPass()`, `isSnp()`, `isIndel()`, `isTransition()`, `isTransversion()`, `findInfo(key): ([]const u8)?`
- **`VcfStats`**：`record_count`, `pass_count`, `snp_count`, `indel_count`, `transitions`, `transversions`, `tiTvRatio(): f64`

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isVcf` | `fn isVcf(data: []const u8): bool` | 校验首行以 `"##fileformat=VCFv"` 开头 |
| `parseVcfHeader` | `fn parseVcfHeader(data: []const u8): VcfHeader?` | 解析版本、`##reference=`、元信息行数、样本数及数据区起始偏移 |
| `VcfRecordIter` | `VcfRecordIter.init(data)` / `next(): VcfRecord?` | 零分配逐行迭代变异记录（跳过 `##`/`#CHROM`/空行，兼容 `\r\n`/`\n`，畸形置 `had_error = true`） |
| `summarizeVcf` | `fn summarizeVcf(data: []const u8): VcfStats` | 单遍统计总变异数、PASS 数、SNP/Indel 数与 Ti/Tv 转换颠换比 |

---

## 使用示例

```typescript
import { parseVcfHeader, VcfRecordIter, summarizeVcf } from "cap:vcf";

fn inspectVcf(vcf_bytes: []const u8) {
    if (hdr := parseVcfHeader(vcf_bytes)) {
        * = hdr.sample_count;
    }
    let it = VcfRecordIter.init(vcf_bytes);
    while (rec := it.next()) {
        if (dp := rec.findInfo("DP")) {
            * = dp;
        }
    }
    const stats = summarizeVcf(vcf_bytes);
    * = stats.tiTvRatio();
}
```
